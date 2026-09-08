package com.reaper.xxx;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.widget.Toast;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

public class Initialize {

    public String daemonEXE = "Fucker";
    public static String socket;
    private static final String TAG = "DAEMON_INIT";

    public Initialize(final Context context) {
        // Load the native library
        System.loadLibrary("joudadooh");

        // 1. Launch the Game
        try {
            PackageManager packageManager = context.getPackageManager();
            Intent launchIntent = packageManager.getLaunchIntentForPackage("com.dts.freefireth");
            if (launchIntent != null) {
                context.startActivity(launchIntent);
            } else {
                Log.e(TAG, "Game package not found!");
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        // 2. Run Setup in Background Thread to prevent "Signal 3" (ANR) crash
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    String outPath = context.getFilesDir().getPath();
                    String daemonPath = outPath + "/" + daemonEXE;

                    // A. KILL existing daemon process to fix "Text file busy" (ETXTBSY)
                    Runtime.getRuntime().exec("su -c killall -9 " + daemonEXE);
                    Thread.sleep(300); // Wait for process to release file handle

                    // B. Copy Binary from Assets
                    boolean success = CopyFromAssets(context, outPath, daemonEXE);
                    
                    if (success) {
                        // C. Set executable permissions
                        Runtime.getRuntime().exec("su -c chmod 777 " + daemonPath).waitFor();

                        // D. Prepare Shell Command
                        socket = "su -c ./ " + daemonPath;

                        // E. Start Daemon in background
                        Runtime.getRuntime().exec("su -c " + daemonPath);

                        // F. Return to Main Thread to show the Floater/Menu
                        new Handler(Looper.getMainLooper()).post(new Runnable() {
                            @Override
                            public void run() {
                                Toast.makeText(context, "Menu Initialized!", Toast.LENGTH_SHORT).show();
                                new Floater(context);
                            }
                        });
                    } else {
                        Log.e(TAG, "Failed to copy daemon from assets");
                    }

                } catch (Exception e) {
                    Log.e(TAG, "Error in background init: " + e.getMessage());
                    e.printStackTrace();
                }
            }
        }).start();
    }

    /**
     * Safely copies the binary from the APK assets to the internal data folder.
     */
    public static boolean CopyFromAssets(Context ctx, String outPath, String fileName) {
        File file = new File(outPath);
        if (!file.exists()) {
            file.mkdirs();
        }
        
        InputStream inputStream = null;
        FileOutputStream fileOutputStream = null;
        
        try {
            inputStream = ctx.getAssets().open(fileName);
            File outFile = new File(file, fileName);
            fileOutputStream = new FileOutputStream(outFile);
            
            byte[] buffer = new byte[4096];
            int byteRead;
            while ((byteRead = inputStream.read(buffer)) != -1) {
                fileOutputStream.write(buffer, 0, byteRead);
            }
            
            fileOutputStream.flush();
            return true;
        } catch (IOException e) {
            Log.e(TAG, "Copy Error: " + e.getMessage());
            return false;
        } finally {
            try {
                if (inputStream != null) inputStream.close();
                if (fileOutputStream != null) fileOutputStream.close();
            } catch (IOException e) {
                // Ignore
            }
        }
    }
}