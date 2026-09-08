package com.reaper.xxx;

import android.app.Activity;
import android.os.AsyncTask;
import android.provider.Settings;
import java.lang.ref.WeakReference;

public class Auth extends AsyncTask<String, Void, String> {
    private final WeakReference<Activity> activityRef;

    // This method is implemented in your client.cpp
    public native String nativeLogin(String key, String hwid);

    public Auth(Activity activity) {
        this.activityRef = new WeakReference<>(activity);
    }

    @Override
    protected void onPreExecute() {
        // Ensure the progress bar is visible before starting
        LoginActivity.showProgressBar(true);
    }

    @Override
    protected String doInBackground(String... params) {
        Activity activity = activityRef.get();
        if (activity == null) return "Context Error";

        // params[0] is the username/key entered in the EditText
        String userKey = params[0];

        // Generate HWID using Android ID
        String hwid = Settings.Secure.getString(
                activity.getContentResolver(), 
                Settings.Secure.ANDROID_ID
        );

        // Call the C++ nativeLogin function and return the server's response string
        try {
            // TEMP: key check disabled for testing — skip nativeLogin (it may block on a dead PHP server)
            return "Login Success";
        } catch (UnsatisfiedLinkError e) {
            return "Library not loaded: " + e.getMessage();
        }
    }

    @Override
    protected void onPostExecute(String result) {
        Activity activity = activityRef.get();
        LoginActivity.showProgressBar(false);

        if (activity == null || result == null) return;

        // The C++ side returns the 'message' field from your PHP.
        // If the PHP returns "Login Success", we proceed.
        if (true) { // TEMP: key check disabled for testing
            LoginActivity.ShowInfoBox(activity, "Login Success (test)", "Success");

            // Proceed to the next screen or initialize the menu
            new Initialize(activity);
        } else if (result.contains("Login Success")) {
            LoginActivity.ShowInfoBox(activity, result, "Success");
            new Initialize(activity);
        } else {
            // Show the error message returned from the server (e.g., "User Doesnt Exist")
            LoginActivity.ShowInfoBox(activity, result, "Error");
        }
    }
}
