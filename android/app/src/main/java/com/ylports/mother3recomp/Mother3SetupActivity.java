package com.ylports.mother3recomp;

import android.app.AlertDialog;
import android.os.Bundle;

import org.gbarecomp.GbaSetupActivity;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

/**
 * Thin MOTHER 3 launcher diagnostic wrapper.
 *
 * Native stdout/stderr are redirected by gbarecomp to
 * files/android-runtime.log. If SDL_main returns or the previous native
 * process died, surface the tail here instead of making PLAY appear to do
 * nothing.
 */
public final class Mother3SetupActivity extends GbaSetupActivity {
    private static final String PREFS = "mother3_diagnostics";
    private static final String LAST_LOG = "last_log_snapshot";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // Avoid a crash/relaunch loop while the Android port is in bring-up.
        getSharedPreferences("setup", MODE_PRIVATE)
            .edit()
            .putBoolean("skip_launcher_on_boot", false)
            .apply();
        super.onCreate(savedInstanceState);
    }

    @Override
    protected void onResume() {
        // Keep automatic relaunch off even if PLAY's shared launcher checkbox
        // wrote it immediately before the native Activity started.
        getSharedPreferences("setup", MODE_PRIVATE)
            .edit()
            .putBoolean("skip_launcher_on_boot", false)
            .apply();

        super.onResume();
        showNativeLogIfChanged();
    }

    private void showNativeLogIfChanged() {
        File log = new File(getFilesDir(), "android-runtime.log");
        if (!log.isFile() || log.length() == 0) return;

        String text;
        try {
            text = readTail(log, 12000);
        } catch (IOException error) {
            return;
        }
        if (text.trim().isEmpty()) return;

        String previous = getSharedPreferences(PREFS, MODE_PRIVATE)
            .getString(LAST_LOG, "");
        if (text.equals(previous)) return;

        getSharedPreferences(PREFS, MODE_PRIVATE)
            .edit()
            .putString(LAST_LOG, text)
            .apply();

        final String visible = lastLines(text, 45);
        new AlertDialog.Builder(this)
            .setTitle("MOTHER 3 — último arranque")
            .setMessage(visible)
            .setPositiveButton("OK", null)
            .show();
    }

    private static String readTail(File file, int maxBytes) throws IOException {
        long length = file.length();
        int count = (int)Math.min(length, maxBytes);
        byte[] data = new byte[count];
        try (FileInputStream input = new FileInputStream(file)) {
            long skip = Math.max(0L, length - count);
            while (skip > 0) {
                long n = input.skip(skip);
                if (n <= 0) break;
                skip -= n;
            }
            int off = 0;
            while (off < data.length) {
                int n = input.read(data, off, data.length - off);
                if (n < 0) break;
                off += n;
            }
        }
        return new String(data, StandardCharsets.UTF_8);
    }

    private static String lastLines(String text, int maxLines) {
        String[] lines = text.split("\\r?\\n");
        int start = Math.max(0, lines.length - maxLines);
        StringBuilder out = new StringBuilder();
        for (int i = start; i < lines.length; ++i) {
            if (lines[i].isEmpty()) continue;
            out.append(lines[i]).append('\n');
        }
        return out.toString();
    }
}
