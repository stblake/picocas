mod commands;
mod picocas;

use commands::{evaluate, load_notebook, plot_data, reset_session, save_notebook, AppState};
use picocas::PicocasSession;
use std::sync::Mutex;
use tauri::Manager;

pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_shell::init())
        .plugin(tauri_plugin_dialog::init())
        .setup(|app| {
            let resource_dir = app
                .path()
                .resource_dir()
                .expect("Failed to get resource dir");

            let session =
                PicocasSession::spawn(&resource_dir).expect("Failed to spawn picocas");

            app.manage(AppState {
                session: Mutex::new(session),
                resource_dir,
            });

            Ok(())
        })
        .invoke_handler(tauri::generate_handler![evaluate, plot_data, reset_session, save_notebook, load_notebook])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
