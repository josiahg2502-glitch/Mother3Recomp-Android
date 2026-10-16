package com.ylports.mother3recomp;

import android.app.AlertDialog;

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

    @Override
    protected boolean supportsAutomaticLaunch() { return false; }

    @Override
    protected void onResume() {
        super.onResume();
        showNativeLogIfChanged();
    }

    private void showNativeLogIfChanged() {
        File status = new File(getFilesDir(), "android-exit-code");
        if (!status.isFile()) return;
        try {
            String code = readTail(status, 32).trim();
            status.delete();
            if (code.equals("0")) return;
        } catch (IOException error) {
            return;
        }
        File log = new File(getFilesDir(), "android-runtime.log");
        if (!log.isFile() || log.length() == 0) return;

        String text;
        try {
            text = readTail(log, 12000);
        } catch (IOException error) {
            return;
        }
        if (text.trim().isEmpty()) return;

        final String visible = lastLines(text, 45);
        new AlertDialog.Builder(this)
            .setTitle("MOTHER 3 - startup failed")
            .setMessage(visible)
            .setNeutralButton("Copy diagnostics", (dialog, which) -> copyDiagnostics())
            .setPositiveButton("OK", null)
            .show();
    }

    @Override
    protected void addGameActions(android.widget.LinearLayout preferences) {
        android.widget.Button button = new android.widget.Button(this);
        button.setText("Copy diagnostics");
        button.setOnClickListener(view -> copyDiagnostics());
        preferences.addView(button);
    }

    private void copyDiagnostics() {
        String text = "MOTHER 3 | " + android.os.Build.MANUFACTURER + " " + android.os.Build.MODEL
            + " | Android " + android.os.Build.VERSION.RELEASE + "\n";
        try {
            text += "App " + getPackageManager().getPackageInfo(getPackageName(), 0).versionName + "\n";
            text += readTail(new File(getFilesDir(), "android-runtime.log"), 12000);
        } catch (Exception error) {
            text += "No startup log available: " + error.getMessage();
        }
        android.content.ClipboardManager clipboard = (android.content.ClipboardManager)
            getSystemService(CLIPBOARD_SERVICE);
        clipboard.setPrimaryClip(android.content.ClipData.newPlainText("MOTHER 3 diagnostics", text));
        android.widget.Toast.makeText(this, "Diagnostics copied", android.widget.Toast.LENGTH_SHORT).show();
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
