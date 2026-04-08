use regex::Regex;
use serde::Serialize;
use std::io::{BufRead, BufReader, Write};
use std::process::{Child, Command, Stdio};
use std::sync::{mpsc, OnceLock};
use std::thread;

static ANSI_RE: OnceLock<Regex> = OnceLock::new();
static OUT_RE: OnceLock<Regex> = OnceLock::new();

fn ansi_re() -> &'static Regex {
    ANSI_RE.get_or_init(|| Regex::new(r"\x1b\[[0-9;]*[a-zA-Z]").unwrap())
}

fn out_re() -> &'static Regex {
    OUT_RE.get_or_init(|| Regex::new(r"^Out\[(\d+)\]= (.*)$").unwrap())
}

#[derive(Debug, Clone, Serialize)]
pub struct EvalResult {
    pub output: String,
    pub out_n: u32,
}

pub struct EvalRequest {
    pub input: String,
    pub tx: tokio::sync::oneshot::Sender<Result<EvalResult, String>>,
}

pub struct PicocasSession {
    pub sender: mpsc::Sender<EvalRequest>,
    child: Child,
}

impl PicocasSession {
    pub fn spawn(resource_dir: &std::path::Path) -> Result<Self, String> {
        let picocas_path = std::env::var("PICOCAS_PATH")
            .map(std::path::PathBuf::from)
            .unwrap_or_else(|_| resource_dir.join("picocas"));
        let wrapper_path = std::env::var("PTY_WRAP_PATH")
            .map(std::path::PathBuf::from)
            .unwrap_or_else(|_| resource_dir.join("pty_wrap.py"));
        // cwd: directory containing picocas (for relative paths it may need)
        let picocas_cwd = picocas_path
            .parent()
            .unwrap_or_else(|| std::path::Path::new("."));

        // Ensure picocas is executable
        #[cfg(unix)]
        {
            use std::os::unix::fs::PermissionsExt;
            if let Ok(mut perms) = std::fs::metadata(&picocas_path).map(|m| m.permissions()) {
                perms.set_mode(perms.mode() | 0o111);
                let _ = std::fs::set_permissions(&picocas_path, perms);
            }
        }

        let mut child = Command::new("python3")
            .args([wrapper_path.to_str().unwrap_or(""), picocas_path.to_str().unwrap_or("")])
            .current_dir(picocas_cwd)
            .stdin(Stdio::piped())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
            .map_err(|e| format!("Failed to spawn picocas: {}", e))?;

        let stdin = child.stdin.take().ok_or("Failed to capture picocas stdin")?;
        let stdout = child.stdout.take().ok_or("Failed to capture picocas stdout")?;

        let (tx, rx) = mpsc::channel::<EvalRequest>();

        // Background thread: drains request queue, writes to stdin, reads stdout
        thread::spawn(move || {
            reader_loop(stdin, stdout, rx);
        });

        Ok(PicocasSession { sender: tx, child })
    }

    pub fn kill(&mut self) {
        let _ = self.child.kill();
        let _ = self.child.wait();
    }
}

impl Drop for PicocasSession {
    fn drop(&mut self) {
        self.kill();
    }
}

/// Strip carriage returns and ANSI escape codes (pty artifacts).
fn clean_line(raw: &str) -> String {
    let no_cr = raw.replace('\r', "");
    ansi_re().replace_all(&no_cr, "").to_string()
}

fn reader_loop(
    mut stdin: std::process::ChildStdin,
    stdout: std::process::ChildStdout,
    rx: mpsc::Receiver<EvalRequest>,
) {
    let reader = BufReader::new(stdout);
    let mut lines_iter = reader.lines();

    for req in rx {
        // Write input to picocas stdin
        let write_result = writeln!(stdin, "{}", req.input).and_then(|_| stdin.flush());
        if let Err(e) = write_result {
            let _ = req.tx.send(Err(format!("Failed to write to picocas: {}", e)));
            continue;
        }

        // Read lines until we get a result or parse error
        let result = read_response(&mut lines_iter);
        let _ = req.tx.send(result);
    }
}

fn read_response(
    lines: &mut impl Iterator<Item = std::io::Result<String>>,
) -> Result<EvalResult, String> {
    let mut collecting = false;
    let mut out_number: u32 = 0;
    let mut result_lines: Vec<String> = Vec::new();

    while let Some(line_result) = lines.next() {
        let raw_line = line_result.map_err(|e| format!("IO error reading picocas stdout: {}", e))?;
        let line = clean_line(&raw_line);

        // Check for Out[N]= result
        if let Some(caps) = out_re().captures(&line) {
            if collecting {
                // Flush previous (shouldn't happen in normal flow)
                let output = result_lines.join("\n");
                return Ok(EvalResult {
                    output,
                    out_n: out_number,
                });
            }
            out_number = caps[1].parse().unwrap_or(0);
            result_lines = vec![caps[2].to_string()];
            collecting = true;
            continue;
        }

        // If collecting, blank line signals end of output block
        if collecting {
            if line.trim().is_empty() {
                let output = result_lines.join("\n");
                return Ok(EvalResult {
                    output,
                    out_n: out_number,
                });
            } else {
                result_lines.push(line);
            }
            continue;
        }

        // Check for Parse error (not inside an Out block)
        if line.trim() == "Parse error" {
            return Err("Parse error".to_string());
        }

        // Ignore other lines (banner, prompts, echoed input, etc.)
    }

    Err("picocas process closed unexpectedly".to_string())
}
