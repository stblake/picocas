use regex::Regex;
use std::sync::{Mutex, OnceLock};
use tauri::State;

use crate::picocas::{EvalRequest, EvalResult, PicocasSession};

static PLOT_RE: OnceLock<Regex> = OnceLock::new();
static PAIR_RE: OnceLock<Regex> = OnceLock::new();

fn plot_re() -> &'static Regex {
    PLOT_RE.get_or_init(|| {
        Regex::new(r"^Plot\[(.+),\s*\{(\w+),\s*(-?[0-9.]+),\s*(-?[0-9.]+)\}\s*\]$").unwrap()
    })
}

fn pair_re() -> &'static Regex {
    PAIR_RE.get_or_init(|| {
        Regex::new(
            r"\{(-?[0-9]+(?:\.[0-9]+)?(?:[eE][+\-]?[0-9]+)?),\s*(-?[0-9]+(?:\.[0-9]+)?(?:[eE][+\-]?[0-9]+)?)\}",
        )
        .unwrap()
    })
}

pub struct AppState {
    pub session: Mutex<PicocasSession>,
    pub resource_dir: std::path::PathBuf,
}

/// Send a single expression to picocas and await the result.
async fn send_to_picocas(input: String, state: &AppState) -> Result<EvalResult, String> {
    let (tx, rx) = tokio::sync::oneshot::channel();
    {
        let session = state.session.lock().map_err(|e| format!("Lock error: {}", e))?;
        session
            .sender
            .send(EvalRequest { input, tx })
            .map_err(|_| "picocas session is closed".to_string())?;
    }
    rx.await.map_err(|_| "picocas did not respond".to_string())?
}

#[tauri::command]
pub async fn evaluate(input: String, state: State<'_, AppState>) -> Result<EvalResult, String> {
    if input.trim().is_empty() {
        return Err("Empty input".to_string());
    }
    send_to_picocas(input, &state).await
}

#[derive(serde::Serialize)]
pub struct PlotData {
    pub points: Vec<[f64; 2]>,
    pub var_name: String,
    pub expr: String,
    pub x_min: f64,
    pub x_max: f64,
}

#[tauri::command]
pub async fn plot_data(input: String, state: State<'_, AppState>) -> Result<PlotData, String> {
    let caps = plot_re()
        .captures(input.trim())
        .ok_or_else(|| "Invalid Plot syntax. Expected: Plot[expr, {var, min, max}]".to_string())?;

    let expr = caps[1].trim().to_string();
    let var_name = caps[2].to_string();
    let x_min: f64 = caps[3].parse().map_err(|_| "Invalid min value".to_string())?;
    let x_max: f64 = caps[4].parse().map_err(|_| "Invalid max value".to_string())?;

    if x_min >= x_max {
        return Err("min must be less than max".to_string());
    }

    let n_points = 100usize;
    let step = (x_max - x_min) / (n_points as f64);
    let table_cmd = format!(
        "Table[{{{}, {}}}, {{{}, {}, {}, {}}}]",
        var_name, expr, var_name, x_min, x_max, step
    );

    let result = send_to_picocas(table_cmd, &state).await?;

    let points: Vec<[f64; 2]> = pair_re()
        .captures_iter(&result.output)
        .filter_map(|c| {
            let x: f64 = c[1].parse().ok()?;
            let y: f64 = c[2].parse().ok()?;
            if y.is_finite() { Some([x, y]) } else { None }
        })
        .collect();

    if points.is_empty() {
        return Err(format!(
            "Could not generate plot data. picocas returned: {}",
            result.output
        ));
    }

    Ok(PlotData { points, var_name, expr, x_min, x_max })
}

#[tauri::command]
pub async fn save_notebook(path: String, content: String) -> Result<(), String> {
    std::fs::write(&path, content).map_err(|e| format!("Failed to save: {}", e))
}

#[tauri::command]
pub async fn load_notebook(path: String) -> Result<String, String> {
    std::fs::read_to_string(&path).map_err(|e| format!("Failed to load: {}", e))
}

#[tauri::command]
pub async fn reset_session(state: State<'_, AppState>) -> Result<(), String> {
    let mut session = state
        .session
        .lock()
        .map_err(|e| format!("Lock error: {}", e))?;
    session.kill();
    let new_session = crate::picocas::PicocasSession::spawn(&state.resource_dir)?;
    *session = new_session;
    Ok(())
}
