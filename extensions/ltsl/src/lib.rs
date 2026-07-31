// LTSL ZED extension: launches the engine-driven LTSL language server.
//
// The language server is a TypeScript LSP (`script/ltsl-lsp/out/server.js`)
// fed by the engine-generated API database (`script/ltsl-lsp/api-database.json`).
// It is not shipped inside the extension -- it lives in the repo the user
// has opened in Zed.
//
// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

use zed_extension_api as zed;

struct LtslExtension;

impl zed::Extension for LtslExtension {
    fn new() -> Self {
        Self
    }

    fn language_server_command(
        &mut self,
        _language_server_id: &zed::LanguageServerId,
        worktree: &zed::Worktree,
    ) -> zed::Result<zed::Command> {
        let node = worktree
            .which("node")
            .ok_or_else(|| "LTSL language server requires `node` on your PATH".to_string())?;

        let root = worktree.root_path();
        let lsp_dir = format!("{}/script/ltsl-lsp", root);
        Ok(zed::Command {
            command: node,
            args: vec![format!("{}/out/server.js", lsp_dir), "--stdio".to_string()],
            env: vec![(
                "LTSL_API_DATABASE".to_string(),
                format!("{}/api-database.json", lsp_dir),
            )],
        })
    }
}

zed::register_extension!(LtslExtension);
