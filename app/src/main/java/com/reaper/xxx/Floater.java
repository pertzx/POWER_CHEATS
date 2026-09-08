package com.reaper.xxx;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.res.ColorStateList;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.graphics.PorterDuff;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.RippleDrawable;
import android.graphics.drawable.StateListDrawable;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.text.Html;
import android.util.Base64;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.CompoundButton;

import java.io.InputStream;
import android.os.AsyncTask;
import java.io.IOException;
import android.content.res.AssetManager;
import com.topjohnwu.superuser.ipc.RootService;
import java.util.Timer;

public class Floater {

    //Callbacks
    private static MSGConnection conn;
    private static Messenger remoteMessenger;
    private static final Messenger myMessenger = new Messenger(new Handler(Looper.getMainLooper(), null));

    //Native Functions
    public static native void Functions();
    public static native void ChangesID(int ID, int Value);
    public static native void ftKZXivSr(Context context);
    public static native String getBase64Icon();

    public static boolean serviceTestQueued;
    private Timer timer;
    public static int SCREEN_WIDTH = 0;
    public static int SCREEN_HEIGHT = 0;

    public static int Width() {
        return SCREEN_WIDTH;
    }

    public static int Height() {
        return SCREEN_HEIGHT;
    }

    public IBinder onBind(Intent intent) {
        return null;
    }
    
    //Variables Menu
    private int buttonClick = 0;
    // Updated to Cyan Accent Color (#00E5FF)
    public static int PrimaryColor = Color.parseColor("#00E5FF");
    
    private static Context context;
    private static Utils utils;
    
    // Layout components for drag and touch
    private FrameLayout rootFrame;
    private RelativeLayout rootRelative;
    private RelativeLayout collapsedRelative;
    private LinearLayout menuLayout;
    private ImageView logoImagem;
    private LinearLayout tabCima;
    private RelativeLayout tabCimaRelative;
    private TextView tituloText;
    private ScrollView funcsList;
    public static LinearLayout funcsListLayout;
    private LinearLayout tabBaixo;
    private RelativeLayout tabBaixoRelative;
    private Button closeButton;

    public Bitmap getBitmapFromAssets(Context context, String fileName) {
        AssetManager assetManager = context.getAssets();
        InputStream istr;
        Bitmap bitmap = null;
        try {
            istr = assetManager.open(fileName);
            bitmap = BitmapFactory.decodeStream(istr);
        } catch (IOException e) {
            e.printStackTrace();
        }
        return bitmap;
    }

    private Bitmap base64ToBitmap(String base64Str) {
        try {
            byte[] decodedBytes = Base64.decode(base64Str, Base64.DEFAULT);
            return BitmapFactory.decodeByteArray(decodedBytes, 0, decodedBytes.length);
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    //Window System Part
    private WindowManager windowManager;
    private WindowManager.LayoutParams windowManagerParams;
    private static boolean isRunning = false;
    
    private class LoadImageTask extends AsyncTask<String, Void, Bitmap> {
        private ImageView imageView;

        public LoadImageTask(ImageView imageView) {
            this.imageView = imageView;
        }

        protected Bitmap doInBackground(String... urls) {
            String url = urls[0];
            Bitmap bitmap = null;
            try {
                InputStream in = new java.net.URL(url).openStream();
                bitmap = BitmapFactory.decodeStream(in);
            } catch (Exception e) {
                e.printStackTrace();
            }
            return bitmap;
        }

        protected void onPostExecute(Bitmap result) {
            if (result != null) {
                imageView.setImageBitmap(result);
            } else {
                // Handle error
            }
        }
    }

    ESPView drawView;
    WindowManager.LayoutParams windowManagerDrawViewParams;
    public static native void PxbftKZXivSr(ESPView espView, Canvas canvas, int width, int height);
    
    public void DrawCanvas() {
        int LAYOUT_FLAG;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            LAYOUT_FLAG = WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY;
        } else {
            LAYOUT_FLAG = WindowManager.LayoutParams.TYPE_PHONE;
        }

        drawView = new ESPView(context);
        windowManagerDrawViewParams = new WindowManager.LayoutParams(-1, -1, this.getLayoutType(), 56, -3);
        windowManagerDrawViewParams.gravity = Gravity.TOP | Gravity.START;
        windowManagerDrawViewParams.x = 0;
        windowManagerDrawViewParams.y = 0;
        windowManagerDrawViewParams.alpha = 0.8f;
        windowManager.addView(drawView, windowManagerDrawViewParams);
    }

    static LinearLayout container_features;

    private int getLayoutType() {
        if (Build.VERSION.SDK_INT >= 26) {
            return 2038;
        }
        if (Build.VERSION.SDK_INT >= 24) {
            return 2002;
        }
        if (Build.VERSION.SDK_INT >= 23) {
            return 2005;
        }
        return 2003;
    }
    
    // Non-static method for instance use
    private int convertDP(int value) {
        return (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, value, context.getResources().getDisplayMetrics());
    }
    
    // Static method for static context use
    private static int convertDPStatic(int value) {
        return (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, value, context.getResources().getDisplayMetrics());
    }
    
    public Floater(Context globContext) {
        context = globContext;
        utils = new Utils(context);
        onCreate();
    }

    public void onCreate() {
        onCreateSystemWindow();
        onCreateTemplate();
    }

    //Criar Template
    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public void onCreateTemplate() {
        // Create main container layouts
        rootFrame = new FrameLayout(context);
        rootFrame.setLayoutParams(new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, 
                ViewGroup.LayoutParams.WRAP_CONTENT));
        
        rootRelative = new RelativeLayout(context);
        rootRelative.setLayoutParams(new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, 
                ViewGroup.LayoutParams.WRAP_CONTENT));
        
        // Collapsed view (icon)
        collapsedRelative = new RelativeLayout(context);
        collapsedRelative.setLayoutParams(new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, 
                ViewGroup.LayoutParams.WRAP_CONTENT));
        collapsedRelative.setAlpha(0.9f);
        collapsedRelative.setVisibility(View.VISIBLE);
        
        // Get base64 icon
        String base64Icon = getBase64Icon();
        logoImagem = new ImageView(context);
        LinearLayout.LayoutParams logoParams = new LinearLayout.LayoutParams(
                convertDP(69), 
                convertDP(59));
        logoImagem.setLayoutParams(logoParams);
        
        if (base64Icon != null && !base64Icon.isEmpty()) {
            Bitmap iconBitmap = base64ToBitmap(base64Icon);
            if (iconBitmap != null) {
                logoImagem.setImageBitmap(iconBitmap);
            } else {
                Bitmap defaultIcon = Bitmap.createBitmap(65, 55, Bitmap.Config.ARGB_8888);
                defaultIcon.eraseColor(PrimaryColor);
                logoImagem.setImageBitmap(defaultIcon);
            }
        } else {
            Bitmap defaultIcon = Bitmap.createBitmap(65, 55, Bitmap.Config.ARGB_8888);
            defaultIcon.eraseColor(PrimaryColor);
            logoImagem.setImageBitmap(defaultIcon);
        }
        
        // Menu layout
        menuLayout = new LinearLayout(context);
        menuLayout.setLayoutParams(new LinearLayout.LayoutParams(
                convertDP(210), 
                ViewGroup.LayoutParams.WRAP_CONTENT));
        menuLayout.setVisibility(View.GONE);
        menuLayout.setAlpha(0.98f);
        menuLayout.setOrientation(LinearLayout.VERTICAL);
        menuLayout.setPadding(0, 0, 0, convertDP(6)); // Bottom padding to keep close button inside UI
        
        // FULL GLASS EFFECT BACKGROUND
        GradientDrawable menuBackground = new GradientDrawable();
        menuBackground.setShape(GradientDrawable.RECTANGLE);
        menuBackground.setColor(Color.parseColor("#3B0F172A")); // Translucent Frosted Glass
        menuBackground.setCornerRadius(convertDP(14));
        menuBackground.setStroke(convertDP(1), Color.parseColor("#4DFFFFFF")); // Subtle Glass Reflection Border
        menuLayout.setBackground(menuBackground);
        
        // Top tab
        tabCima = new LinearLayout(context);
        tabCima.setLayoutParams(new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, 
                convertDP(40)));
        
        tabCimaRelative = new RelativeLayout(context);
        tabCimaRelative.setLayoutParams(new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, 
                convertDP(40)));
        tabCimaRelative.setGravity(Gravity.LEFT);
        
        // Title text
        tituloText = new TextView(context);
        tituloText.setLayoutParams(new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, 
                convertDP(40)));
        tituloText.setText("B2K-X-LIVE");
        tituloText.setTextColor(PrimaryColor);
        tituloText.setTypeface(null, Typeface.BOLD);
        tituloText.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18);
        tituloText.setGravity(Gravity.CENTER);
        tituloText.setShadowLayer(convertDP(2), convertDP(2), convertDP(2), Color.BLACK);
        
        // Functions list - SLIGHTLY INCREASED HEIGHT
        funcsList = new ScrollView(context);
        funcsList.setLayoutParams(new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, 
                convertDP(190))); // Increased from 180
        
        funcsListLayout = new LinearLayout(context);
        funcsListLayout.setLayoutParams(new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, 
                convertDP(190))); // Increased from 180
        funcsListLayout.setOrientation(LinearLayout.VERTICAL);
        // 5DP VERTICAL PADDING
        funcsListLayout.setPadding(0, convertDP(5), 0, convertDP(5));
        
        container_features = funcsListLayout;
        
        // Bottom tab - PROPER INNER BOUNDS TO CONTAIN CLOSE BUTTON
        tabBaixo = new LinearLayout(context);
        tabBaixo.setLayoutParams(new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, 
                convertDP(36))); 
        tabBaixo.setGravity(Gravity.CENTER_VERTICAL);
        
        tabBaixoRelative = new RelativeLayout(context);
        RelativeLayout.LayoutParams tabBaixoParams = new RelativeLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, 
                convertDP(36));
        tabBaixoRelative.setLayoutParams(tabBaixoParams);
        
        // Close button - FULL GLASS BORDER STYLE (FITTED INSIDE UI)
        GradientDrawable closeButtonBackground = new GradientDrawable();
        closeButtonBackground.setShape(GradientDrawable.RECTANGLE);
        closeButtonBackground.setColor(Color.parseColor("#2B151D2A")); // Translucent Glass
        closeButtonBackground.setStroke(convertDP(1), Color.parseColor("#00E5FF")); // Neon Cyan Border
        closeButtonBackground.setCornerRadius(convertDP(8));
        
        closeButton = new Button(context);
        RelativeLayout.LayoutParams closeParams = new RelativeLayout.LayoutParams(
                convertDP(95), 
                convertDP(28));
        closeParams.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
        closeParams.addRule(RelativeLayout.CENTER_VERTICAL);
        closeParams.setMargins(0, 0, convertDP(8), 0); // Right Margin to keep inside glass background
        closeButton.setLayoutParams(closeParams);
        
        closeButton.setText("CLOSE");
        closeButton.setTextColor(Color.WHITE);
        closeButton.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        closeButton.setAllCaps(false);
        closeButton.setTypeface(null, Typeface.BOLD);
        closeButton.setPadding(0, 0, 0, 0); // NO PADDING
        closeButton.setVisibility(View.VISIBLE);
        closeButton.setBackground(closeButtonBackground);
        closeButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                collapsedRelative.setVisibility(View.VISIBLE);
                menuLayout.setVisibility(View.GONE);
            }
        });
        
        // Build hierarchy
        rootFrame.addView(rootRelative);
        rootRelative.addView(collapsedRelative);
        collapsedRelative.addView(logoImagem);
        rootRelative.addView(menuLayout);
        
        menuLayout.addView(tabCima);
        tabCima.addView(tabCimaRelative);
        tabCimaRelative.addView(tituloText);
        
        menuLayout.addView(funcsList);
        funcsList.addView(funcsListLayout);
        
        menuLayout.addView(tabBaixo);
        tabBaixo.addView(tabBaixoRelative);
        tabBaixoRelative.addView(closeButton);
        
        // Add rootFrame to window manager
        windowManager.addView(rootFrame, windowManagerParams);
        
        // Set touch listeners
        rootFrame.setOnTouchListener(createOnTouchListener());
        logoImagem.setOnTouchListener(createOnTouchListener());
        logoImagem.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                collapsedRelative.setVisibility(View.GONE);
                menuLayout.setVisibility(View.VISIBLE);
            }
        });
        
        // Call functions after delay
        Handler handler = new Handler(Looper.getMainLooper());
        handler.postDelayed(new Runnable() {
            @Override
            public void run() {
                Functions();
                ftKZXivSr(context);
            }
        }, 5000);
    }

    //Criar Sistema Da Janela
    @SuppressLint("ClickableViewAccessibility")
    public void onCreateSystemWindow() {
        int LAYOUT_FLAG;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            LAYOUT_FLAG = WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY;
        } else {
            LAYOUT_FLAG = WindowManager.LayoutParams.TYPE_PHONE;
        }

        windowManagerParams = new WindowManager.LayoutParams(
                WindowManager.LayoutParams.WRAP_CONTENT, 
                WindowManager.LayoutParams.WRAP_CONTENT,
                LAYOUT_FLAG, 
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
                PixelFormat.TRANSLUCENT);
        windowManagerParams.gravity = Gravity.TOP | Gravity.LEFT;
        windowManagerParams.x = 0;
        windowManagerParams.y = 100;

        windowManager = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        DrawCanvas();
    }

    // Create touch listener for dragging
    private View.OnTouchListener createOnTouchListener() {
        return new View.OnTouchListener() {
            private int initialX;
            private int initialY;
            private float initialTouchX;
            private float initialTouchY;

            @Override
            public boolean onTouch(View v, MotionEvent event) {
                switch (event.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        initialX = windowManagerParams.x;
                        initialY = windowManagerParams.y;
                        initialTouchX = event.getRawX();
                        initialTouchY = event.getRawY();
                        return true;
                        
                    case MotionEvent.ACTION_MOVE:
                        windowManagerParams.x = initialX + (int) (event.getRawX() - initialTouchX);
                        windowManagerParams.y = initialY + (int) (event.getRawY() - initialTouchY);
                        windowManager.updateViewLayout(rootFrame, windowManagerParams);
                        return true;
                        
                    case MotionEvent.ACTION_UP:
                        int xDiff = (int) (event.getRawX() - initialTouchX);
                        int yDiff = (int) (event.getRawY() - initialTouchY);
                        
                        // If movement is small, treat as click
                        if (Math.abs(xDiff) < 10 && Math.abs(yDiff) < 10) {
                            if (v == logoImagem) {
                                collapsedRelative.setVisibility(View.GONE);
                                menuLayout.setVisibility(View.VISIBLE);
                            }
                        }
                        return true;
                }
                return false;
            }
        };
    }
    
    // Layout methods - 5DP SIDE SPACING FOR ALL
    public static void addCategory(String name) {
        addCategory(name, 26);
    }
    
    public static void addCategory(String name, int categoryHeightDp) {
        // Glassmorphism Card Style Header
        GradientDrawable gradientDrawable = new GradientDrawable();
        gradientDrawable.setShape(GradientDrawable.RECTANGLE);
        gradientDrawable.setColor(Color.parseColor("#3B1E293B")); // Translucent Dark Glass
        gradientDrawable.setStroke(convertDPStatic(1), Color.parseColor("#00E5FF")); // Border Accent
        gradientDrawable.setCornerRadius(convertDPStatic(8));
        
        TextView textView = new TextView(context);
        textView.setBackground(gradientDrawable);
        textView.setText("  " + name);
        textView.setGravity(Gravity.CENTER_VERTICAL);
        textView.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        textView.setTextColor(Color.parseColor("#00E5FF"));
        textView.setTypeface(textView.getTypeface(), Typeface.BOLD);
        textView.setPadding(convertDPStatic(8), 0, 0, 0);
        
        LinearLayout.LayoutParams layoutParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, 
                convertDPStatic(categoryHeightDp)
        );
        
        // 5DP SIDE SPACING
        layoutParams.setMargins(convertDPStatic(5), convertDPStatic(4), convertDPStatic(5), convertDPStatic(4));
        textView.setLayoutParams(layoutParams);
        
        container_features.addView(textView);
    }

    public static void addSwitch(final String name, final int ID) {
        addSwitch(name, ID, 38);
    }

    public static void addSwitch(final String name, final int ID, int switchHeightDp) {
        // Check if we're on the UI thread
        if (Looper.myLooper() != Looper.getMainLooper()) {
            // Run on UI thread if not already there
            new Handler(Looper.getMainLooper()).post(new Runnable() {
                @Override
                public void run() {
                    addSwitch(name, ID, switchHeightDp);
                }
            });
            return;
        }
        
        try {
            if (remoteMessenger == null) {
                serviceTestQueued = true;
                Intent intent = new Intent(context, RootServices.class);
                MSGConnection mSGConnection = new MSGConnection();
                conn = mSGConnection;
                RootService.bind(intent, (ServiceConnection) mSGConnection);
            }

            // Glassmorphism Switch Card Container
            final LinearLayout switchContainer = new LinearLayout(context);
            LinearLayout.LayoutParams containerParams = new LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT, 
                    convertDPStatic(switchHeightDp)
            );
            containerParams.setMargins(convertDPStatic(5), convertDPStatic(3), convertDPStatic(5), convertDPStatic(3));
            switchContainer.setLayoutParams(containerParams);
            switchContainer.setOrientation(LinearLayout.HORIZONTAL);
            switchContainer.setGravity(Gravity.CENTER_VERTICAL);
            switchContainer.setPadding(convertDPStatic(10), 0, convertDPStatic(10), 0);
            
            // OFF Glass State (Subtle Border)
            final GradientDrawable offDrawable = new GradientDrawable();
            offDrawable.setShape(GradientDrawable.RECTANGLE);
            offDrawable.setColor(Color.parseColor("#2B151D2A")); // Glass Translucent Dark
            offDrawable.setStroke(convertDPStatic(1), Color.parseColor("#33FFFFFF")); // Light Border
            offDrawable.setCornerRadius(convertDPStatic(10));
            
            // ON Glass State (Neon Cyan Border)
            final GradientDrawable onDrawable = new GradientDrawable();
            onDrawable.setShape(GradientDrawable.RECTANGLE);
            onDrawable.setColor(Color.parseColor("#3B151D2A"));
            onDrawable.setStroke(convertDPStatic(1), Color.parseColor("#00E5FF")); // Bright Border
            onDrawable.setCornerRadius(convertDPStatic(10));

            switchContainer.setBackground(offDrawable);
            
            // Create text view for switch label
            final TextView switchText = new TextView(context);
            LinearLayout.LayoutParams textParams = new LinearLayout.LayoutParams(
                    0, 
                    LinearLayout.LayoutParams.WRAP_CONTENT,
                    1.0f
            );
            switchText.setLayoutParams(textParams);
            switchText.setText(name);
            switchText.setTextColor(Color.parseColor("#B0BEC5"));
            switchText.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
            switchText.setTypeface(null, Typeface.BOLD);
            
            // Create NORMAL SWITCH
            final android.widget.Switch normalSwitch = new android.widget.Switch(context);
            LinearLayout.LayoutParams switchParams = new LinearLayout.LayoutParams(
                    convertDPStatic(50), 
                    convertDPStatic(28)
            );
            normalSwitch.setLayoutParams(switchParams);
            
            // Set switch colors - Cyan when ON
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                int[][] states = new int[][] {
                    new int[] { android.R.attr.state_checked },
                    new int[] { -android.R.attr.state_checked }
                };
                
                int[] trackColors = new int[] {
                    Color.parseColor("#00E5FF"),
                    Color.parseColor("#37474F")
                };
                
                int[] thumbColors = new int[] {
                    Color.WHITE,
                    Color.parseColor("#78909C")
                };
                
                ColorStateList trackColorList = new ColorStateList(states, trackColors);
                ColorStateList thumbColorList = new ColorStateList(states, thumbColors);
                
                normalSwitch.setTrackTintList(trackColorList);
                normalSwitch.setThumbTintList(thumbColorList);
                normalSwitch.setThumbTintMode(PorterDuff.Mode.SRC_IN);
            }
            
            normalSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                    ChangesID(ID, isChecked ? 1 : 0);
                    if (isChecked) {
                        switchContainer.setBackground(onDrawable);
                        switchText.setTextColor(Color.parseColor("#00E5FF"));
                    } else {
                        switchContainer.setBackground(offDrawable);
                        switchText.setTextColor(Color.parseColor("#B0BEC5"));
                    }
                }
            });
            
            // Add views to container
            switchContainer.addView(switchText);
            switchContainer.addView(normalSwitch);
            
            if (container_features != null) {
                container_features.addView(switchContainer);
            }
            
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static void addSeekBar(final String name, int value, int max, final String type, final int ID) {
        LinearLayout linearLayout = new LinearLayout(context);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        params.setMargins(convertDPStatic(5), convertDPStatic(3), convertDPStatic(5), convertDPStatic(3));
        linearLayout.setLayoutParams(params);
        linearLayout.setPadding(convertDPStatic(8), convertDPStatic(6), convertDPStatic(8), convertDPStatic(6));
        linearLayout.setOrientation(LinearLayout.VERTICAL);

        // Glass background for SeekBar
        GradientDrawable cardBg = new GradientDrawable();
        cardBg.setShape(GradientDrawable.RECTANGLE);
        cardBg.setColor(Color.parseColor("#2B151D2A"));
        cardBg.setStroke(convertDPStatic(1), Color.parseColor("#33FFFFFF"));
        cardBg.setCornerRadius(convertDPStatic(10));
        linearLayout.setBackground(cardBg);

        final TextView textView = new TextView(context);
        textView.setText(name.concat(": ") + value + type);
        textView.setTextSize(11);
        textView.setTextColor(Color.WHITE);
        textView.setTypeface(null, Typeface.BOLD); 
        
        if (type.equals("Color")) {
            if(value == 0) {
                textView.setText(Html.fromHtml(name + ": <font color='#ffffff'>" + "Branco" + "</font>"));
            } else if(value == 1) {
                textView.setText(Html.fromHtml(name + ": <font color='#39FF14'>" + "Verde" + "</font>"));
            } else if(value == 2) {
                textView.setText(Html.fromHtml(name + ": <font color='#0000FF'>" + "Azul" + "</font>"));
            } else if(value == 3) {
                textView.setText(Html.fromHtml(name + ": <font color='#FF0000'>" + "Vermelho" + "</font>"));
            } else if(value == 4) {
                textView.setText(Html.fromHtml(name + ": <font color='#000000'>" + "Preto" + "</font>"));
            } else if(value == 5) {
                textView.setText(Html.fromHtml(name + ": <font color='#FFFF00'>" + "Amarelo" + "</font>"));
            } else if(value == 6) {
                textView.setText(Html.fromHtml(name + ": <font color='#00FFFF'>" + "Ciano" + "</font>"));
            } else if(value == 7) {
                textView.setText(Html.fromHtml(name + ": <font color='#FF00FF'>" + "Magenta" + "</font>"));
            } else if(value == 8) {
                textView.setText(Html.fromHtml(name + ": <font color='#808080'>" + "Cinza" + "</font>"));
            } else if(value == 9) {
                textView.setText(Html.fromHtml(name + ": <font color='#A020F0'>" + "Roxo" + "</font>"));
            }
        } else if (type.equals("BoxType")) {
            if (value == 0) {
                textView.setText(name.concat(": Stroke"));
            } else if (value == 1) {
                textView.setText(name.concat(": Filled"));
            } else if (value == 2) {
                textView.setText(name.concat(": Rounded"));
            }
        } else if (type.equals("LineType")) {
            if (value == 0) {
                textView.setText(name.concat(": Top"));
            } else if (value == 1) {
                textView.setText(name.concat(": Center"));
            } else if (value == 2) {
                textView.setText(name.concat(": Bottom"));
            }
        }

        SeekBar seekBar = new SeekBar(context);
        seekBar.getThumb().setColorFilter(PrimaryColor, PorterDuff.Mode.SRC_IN);
        seekBar.getProgressDrawable().setColorFilter(PrimaryColor, PorterDuff.Mode.SRC_IN);
        seekBar.setProgress(value);
        seekBar.setMax(max);
        if (type.equals("Color")) {
            seekBar.setMax(9);
        } else if (type.equals("BoxType")) {
            seekBar.setMax(2);
        } else if (type.equals("LineType")) {
            seekBar.setMax(2);
        }

        seekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int i, boolean b) {
                if (type.equals("Color")) {
                    if(i == 0) {
                        textView.setText(Html.fromHtml(name + ": <font color='#ffffff'>" + "Branco" + "</font>"));
                    } else if(i == 1) {
                        textView.setText(Html.fromHtml(name + ": <font color='#39FF14'>" + "Verde" + "</font>"));
                    } else if(i == 2) {
                        textView.setText(Html.fromHtml(name + ": <font color='#0000FF'>" + "Azul" + "</font>"));
                    } else if(i == 3) {
                        textView.setText(Html.fromHtml(name + ": <font color='#FF0000'>" + "Vermelho" + "</font>"));
                    } else if(i == 4) {
                        textView.setText(Html.fromHtml(name + ": <font color='#000000'>" + "Preto" + "</font>"));
                    } else if(i == 5) {
                        textView.setText(Html.fromHtml(name + ": <font color='#FFFF00'>" + "Amarelo" + "</font>"));
                    } else if(i == 6) {
                        textView.setText(Html.fromHtml(name + ": <font color='#00FFFF'>" + "Ciano" + "</font>"));
                    } else if(i == 7) {
                        textView.setText(Html.fromHtml(name + ": <font color='#FF00FF'>" + "Magenta" + "</font>"));
                    } else if(i == 8) {
                        textView.setText(Html.fromHtml(name + ": <font color='#808080'>" + "Cinza" + "</font>"));
                    } else if(i == 9) {
                        textView.setText(Html.fromHtml(name + ": <font color='#A020F0'>" + "Roxo" + "</font>"));
                    }
                } else if (type.equals("BoxType")) {
                    if (i == 0) {
                        textView.setText(name.concat(": Stroke"));
                    } else if (i == 1) {
                        textView.setText(name.concat(": Filled"));
                    } else if (i == 2) {
                        textView.setText(name.concat(": Corner"));
                    }
                } else if (type.equals("LineType")) {
                    if (i == 0) {
                        textView.setText(name.concat(": Top"));
                    } else if (i == 1) {
                        textView.setText(name.concat(": Center"));
                    } else if (i == 2) {
                        textView.setText(name.concat(": Bottom"));
                    }
                } else {
                    textView.setText(name.concat(": ") + i + type);
                }

                ChangesID(ID, i);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });

        container_features.addView(linearLayout);
        linearLayout.addView(textView);
        linearLayout.addView(seekBar);
    }

    public static void Funçoes(String pkg, String lib, String offset, String hex) {
        Message message = Message.obtain(null, RootServices.MSG_GETINFO);
        message.getData().putString("pkg", pkg);
        message.getData().putString("fileSo", lib);
        message.getData().putInt("offset", Integer.decode(offset).intValue());
        message.getData().putString("hexNumber", hex);
        message.replyTo = myMessenger;
        try {
            remoteMessenger.send(message);
        } catch (RemoteException e) {
        }
    }

    public static class MSGConnection implements ServiceConnection {
        public void onServiceConnected(ComponentName name, IBinder service) {
            setRemoteMessenger(new Messenger(service));
            if (serviceTestQueued) {
                serviceTestQueued = false;
            }
        }

        public void onServiceDisconnected(ComponentName name) {
            setRemoteMessenger((Messenger) null);
        }
    }

    public static final void setRemoteMessenger(Messenger messenger) {
        remoteMessenger = messenger;
    }
}
