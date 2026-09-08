#include <pthread.h>
#include <jni.h>
#include <src/Socket/client.h>
#include "src/Includes/obfuscate.h"
#include <sys/stat.h>
#include <src/Widgets/ImportWidgets.h>
#include <src/Includes/http.h>
#include "src/Unity/Vector3.hpp"
#include "src/Unity/Vector2.hpp"
#include "src/Unity/Unity.h"
#include "src/Unity/ESP.h"
#include "Includes/Logger.h"
#include "Includes/Utils.h"
#include "src/Widgets/Methods.h"

#include "Client.h"
#include "BooleanClient.h"


#include <curl/curl.h>
#include "src/Includes/json.h" 
using json = nlohmann::json;

struct MemoryChunk {
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryChunk *mem = (struct MemoryChunk *)userp;
    char *ptr = (char *)realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) return 0;
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

std::string XOR_decryption(std::string value, std::string key) {
    std::string text = "";
    int klen = key.length();
    for (int v = 0; v < value.length(); v++) {
        text += value[v] ^ key[v % klen];
    }
    return text;
}

std::string get_Key_From_Public(std::string publicKey) {
    if (publicKey.length() < 98) return "000000000";
    std::string keyDes = "";
    keyDes += publicKey[58 - 1];
    keyDes += publicKey[62 - 1];
    keyDes += publicKey[10 - 1];
    keyDes += publicKey[47 - 1];
    keyDes += publicKey[20 - 1];
    keyDes += publicKey[33 - 1];
    keyDes += publicKey[87 - 1];
    keyDes += publicKey[98 - 1];
    keyDes += publicKey[14 - 1];
    return keyDes;
}

#define game_package OBFUSCATE("com.dts.freefireth")
#define LibBypass OBFUSCATE("libanogs.so")

struct {
    bool connectClient = false;
    std::string server_msg = "Waiting..."; 
} Connection;

ESP espOverlay;

void DrawESP(ESP esp) {
    auto screenHeight = esp.getHeight();
    auto screenWidth = esp.getWidth();
    if (!isConnected()) return;

    auto response = getData(screenWidth, screenHeight);
    if (!response.Success || response.PlayerCount <= 0) return;

    float globalThickness = 1.0f;

    for (int i = 0; i < response.PlayerCount; i++) {
        auto player = response.Players[i];
        if (player.Head == Vector3::Zero() || player.Toe == Vector3::Zero()) continue;
        if (!Actived.activar) continue;

        float distance = player.Distancia;
        if (distance < 0.1f || distance > 150.0f) continue;

        // --- POSITIONING ---
        float headToFeetHeight = std::abs(player.Toe.Y - player.Head.Y);
        float boxHeight = headToFeetHeight * 1.15f; 
        float width = boxHeight * 0.48f;
        float boxY = player.Head.Y - (boxHeight * 0.12f);
        float boxX = player.Head.X - (width / 2.0f);

        Color currentBoxColor = player.IsCaido ? Color::Red() : pEspPlayer.espColor;
        Color strokeColor = Color(0, 0, 0, 255);
        float fSize = pEspPlayer.textSize; // This is controlled by seekbar
        
                // --- CENTER ALIGNMENT LOGIC ---
        float offsetMinus = 60.0f; // Aapka custom offset
        float offsetPlus  = 0.0f;
        float centerX = (screenWidth * 0.5f) - offsetMinus + offsetPlus;
        float shiftX = (centerX - screenWidth * 0.5f);

        // --- DRAW LINE ---
        if (pEspPlayer.espLinha) {
            Vector3 lineStart, lineEnd;
            if (pEspPlayer.lineType == LINE_TOP) lineStart = Vector3(centerX, 0, 0);
            else if (pEspPlayer.lineType == LINE_CENTER) lineStart = Vector3(centerX, screenHeight / 2, 0);
            else lineStart = Vector3(centerX, screenHeight, 0);

            lineEnd = (pEspPlayer.lineType == LINE_BOTTOM) ? Vector3(player.Head.X + shiftX, boxY + boxHeight, 0) : Vector3(player.Head.X + shiftX, boxY, 0);
            esp.DrawLine(currentBoxColor, globalThickness, lineStart, lineEnd);
        }

        // --- DRAW BOX ---
        if (pEspPlayer.espCaixa) {
            Rect playerRect(boxX + shiftX, boxY, width, boxHeight);
            if (pEspPlayer.boxType == BOX_FILLED || pEspPlayer.boxType == BOX_ROUNDED) {
                Color fill = currentBoxColor; fill.a = 70;
                esp.DrawFilledRect(fill, Vector3(boxX + shiftX, boxY, 0), width, boxHeight);
            }
            if (pEspPlayer.boxType == BOX_CORNER) esp.DrawBox4Line(boxX + shiftX, boxY, width, boxHeight, currentBoxColor, globalThickness);
            else esp.DrawBox(currentBoxColor, globalThickness, playerRect);
        }

        // --- DRAW HEALTH ---
        if (pEspPlayer.espHealth) {
            float hBarW = (distance > 80.0f) ? 4.5f : 6.0f; 
            float hBarX = (boxX + shiftX) - (hBarW + 6);
            float hMax = player.Name ? 100.0f : 200.0f; 
            esp.DrawVerticalHealthBar(Vector2(hBarX, boxY), boxHeight, hMax, player.Health, hBarW, false);
        }

        // --- DRAW PLAYER NAME ---
        if (pEspPlayer.espName && player.PlayerName[0] != '\0') {
            float fSizeName = fSize;
            float nameY = boxY - fSizeName;
            
            if (player.Head.X + shiftX > 0 && player.Head.X + shiftX < screenWidth) {
                esp.DrawText(strokeColor, player.PlayerName, Vector2(player.Head.X + shiftX - 1, nameY - 1), fSizeName);
                esp.DrawText(strokeColor, player.PlayerName, Vector2(player.Head.X + shiftX + 1, nameY + 1), fSizeName);
                esp.DrawText(strokeColor, player.PlayerName, Vector2(player.Head.X + shiftX - 1, nameY + 1), fSizeName);
                esp.DrawText(strokeColor, player.PlayerName, Vector2(player.Head.X + shiftX + 1, nameY - 1), fSizeName);
                
                esp.DrawText(currentBoxColor, player.PlayerName, Vector2(player.Head.X + shiftX, nameY), fSizeName);
            }
        }

        // --- DRAW DISTANCE ---
        if (pEspPlayer.espDistance && !player.IsCaido) {
            std::string dStr = std::to_string((int)distance) + "M";
            float dY = boxY + boxHeight + fSize + 8; 

            esp.DrawText(strokeColor, dStr.c_str(), Vector2(player.Head.X + shiftX - 1, dY - 1), fSize);
            esp.DrawText(strokeColor, dStr.c_str(), Vector2(player.Head.X + shiftX + 1, dY + 1), fSize);
            esp.DrawText(strokeColor, dStr.c_str(), Vector2(player.Head.X + shiftX - 1, dY + 1), fSize);
            esp.DrawText(strokeColor, dStr.c_str(), Vector2(player.Head.X + shiftX + 1, dY - 1), fSize);
            esp.DrawText(currentBoxColor, dStr.c_str(), Vector2(player.Head.X + shiftX, dY), fSize);
        }

    }
}



extern "C"
JNIEXPORT void JNICALL
Java_com_reaper_xxx_Floater_Functions(JNIEnv *env, jclass clazz) {
    Widget widget = Widget(env);

    widget.Category(OBFUSCATE("Memory Menu"));
    widget.Switch(OBFUSCATE("Enable Functions"), 1);
   widget.Switch(OBFUSCATE("Aim bot"), 2);
  //  widget.Switch(OBFUSCATE("Aim Silent"), 8);
    widget.SeekBar(OBFUSCATE("Aim Fov"), 0, 360, OBFUSCATE(""), 9);
  //  widget.Switch(OBFUSCATE("Aim collder"), 6);
    
    // CHANGE FROM SEEKBAR TO SWITCH FOR SPEED
 //   widget.Category(OBFUSCATE("Speed Menu"));
  //  widget.Switch(OBFUSCATE("Speed Hack"), 25); // ID 25 for speed toggle
    
    // REMOVE THE SEEKBAR LINE:
    // widget.SeekBar(OBFUSCATE("Game Speed"), 10, 50, OBFUSCATE(""), 25);

    widget.Category(OBFUSCATE("Esp Menu"));
    widget.Switch(OBFUSCATE("Draw Line"), 11);
    widget.Switch(OBFUSCATE("Draw Box"), 12);
    widget.Switch(OBFUSCATE("Draw Distance"), 13);
    widget.Switch(OBFUSCATE("Draw Name"), 21);
    widget.Switch(OBFUSCATE("Draw Health"), 19);
    
    widget.Category(OBFUSCATE("Esp Config"));
    widget.SeekBar(OBFUSCATE("ESP Color"), 0, 9, OBFUSCATE("Color"), 16);
    widget.SeekBar(OBFUSCATE("Box Type"), 0, 3, OBFUSCATE("BoxType"), 17);
    widget.SeekBar(OBFUSCATE("Line Type"), 0, 2, OBFUSCATE("LineType"), 18);
    widget.SeekBar(OBFUSCATE("Text Size"), 10, 15, OBFUSCATE(""), 20);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_reaper_xxx_Floater_ChangesID(JNIEnv *env, jclass clazz, jint id, jint value) {
    switch (id) {
        case 1: Actived.activar = !Actived.activar; SendBool(3, Actived.activar); break;
        case 2: pPlayer.aimbot = !pPlayer.aimbot; SendBool(5, pPlayer.aimbot); break;
     //   case 3: pPlayer.fovawm = !pPlayer.fovawm; SendBool(22, pPlayer.fovawm); break;
     //   case 6: pPlayer.aimbotlock = !pPlayer.aimbotlock; SendBool(54, pPlayer.aimbotlock); break;
      //  case 8: pPlayer.silen4a = !pPlayer.silen4a; SendBool(57, pPlayer.silen4a); break;
        case 9: pPlayer.AimFov = value; SendFloat(58, pPlayer.AimFov); break;
       // case 10: pPlayer.AimDistance = value; SendFloat(59, pPlayer.AimDistance); break;
        case 11: pEspPlayer.espLinha = !pEspPlayer.espLinha; break;
        case 12: pEspPlayer.espCaixa = !pEspPlayer.espCaixa; break;
        case 13: pEspPlayer.espDistance = !pEspPlayer.espDistance; break;
     //   case 14: pEspPlayer.espSkeleton = !pEspPlayer.espSkeleton; break;
      //  case 15: pEspPlayer.espIden360 = !pEspPlayer.espIden360; break;
        case 16:
            if (value == 0) pEspPlayer.espColor = Color(255, 255, 255, 255);
            else if (value == 1) pEspPlayer.espColor = Color(0, 255, 0, 255);
            else if (value == 2) pEspPlayer.espColor = Color(0, 0, 255, 255);
            else if (value == 3) pEspPlayer.espColor = Color(255, 0, 0, 255);
            else if (value == 4) pEspPlayer.espColor = Color(0, 0, 0, 255);
            else if (value == 5) pEspPlayer.espColor = Color(255, 255, 0, 255);
            else if (value == 6) pEspPlayer.espColor = Color(0, 255, 255, 255);
            else if (value == 7) pEspPlayer.espColor = Color(255, 0, 255, 255);
            else if (value == 8) pEspPlayer.espColor = Color(128, 128, 128, 255);
            else if (value == 9) pEspPlayer.espColor = Color(160, 32, 240, 255);
            break;
        case 17: pEspPlayer.boxType = static_cast<BoxType>(value); break;
        case 18: pEspPlayer.lineType = static_cast<LineType>(value); break;
        case 19: pEspPlayer.espHealth = !pEspPlayer.espHealth; break;
        case 20: pEspPlayer.textSize = (float)value; break;
        case 21: pEspPlayer.espName = !pEspPlayer.espName; break;
        // CHANGE SPEED CONTROL TO TOGGLE
        case 25: 
            pPlayer.SpeedHackV2 = !pPlayer.SpeedHackV2; // Toggle speed hack on/off
            SendBool(60, pPlayer.SpeedHackV2); // Send boolean instead of float
            break;
    }
}


extern "C" JNIEXPORT void JNICALL
Java_com_reaper_xxx_Floater_PxbftKZXivSr(JNIEnv *env, jclass type, jobject espView, jobject canvas, jint width, jint height) {
    ESP localEsp = ESP(env, espView, canvas);
    if (localEsp.isValid()) { DrawESP(localEsp); }
}

extern "C" JNIEXPORT void JNICALL
Java_com_reaper_xxx_Floater_ftKZXivSr(JNIEnv *env, jclass clazz, jobject ctx) {
    if (!Connection.connectClient) {
        startClient();
        Connection.connectClient = true;
        Toast(env, ctx, "ACTIVATING...", 1, 1);
        Toast(env, ctx, "ACTIVATED", 1, 1);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_reaper_xxx_Floater_ftKwCvSr(JNIEnv *env, jclass clazz) {
    stopClient();
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_reaper_xxx_LoginActivity_apkHashUrl(JNIEnv *env, jobject thiz) {
    return env->NewStringUTF("https://rewardff.com.br/apkhash.php");
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_reaper_xxx_LoginActivity_updateUrl(JNIEnv *env, jobject thiz) {
    return env->NewStringUTF("https://rewardff.com.br/update.php");
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_reaper_xxx_Auth_URL(JNIEnv *env, jobject thiz) {
    return env->NewStringUTF("https://viku.urlking.in/connect/b2k");
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_reaper_xxx_Auth_nativeLogin(JNIEnv *env, jobject thiz, jstring uKey, jstring uHwid) {
    const char *key = env->GetStringUTFChars(uKey, 0);
    const char *hwid = env->GetStringUTFChars(uHwid, 0);

    CURL *curl;
    struct MemoryChunk chunk;
    chunk.memory = (char *)malloc(1);
    chunk.size = 0;

    curl = curl_easy_init();
    if (curl) {
        std::string jsonStr = "game=PUBG&user_key=" + std::string(key) + "&serial=" + std::string(hwid);

        curl_easy_setopt(curl, CURLOPT_URL, "https://viku.urlking.in/connect/b2k");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
        
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
        headers = curl_slist_append(headers, "X-API-Key: X7B4N2P8Q9W3Z6M5"); 
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

        if (curl_easy_perform(curl) == CURLE_OK) {
            try {
                json response = json::parse(chunk.memory);
                
                bool status = false;
                if (response.find("status") != response.end() && response["status"].is_boolean()) {
                    status = response["status"].get<bool>();
                }

                if (status == true) {
                    if (response.find("data") != response.end() && response["data"].find("token") != response["data"].end()) {
                        std::string serverToken = response["data"]["token"].get<std::string>();

                        std::string realString = "PUBG-" + std::string(key) + "-" + std::string(hwid) + "-Vm8Lk7Uj2JmsjCPVPVjrLa7zgfx3uz9E";
                        
                        // --- FIXED: INLINE STANDARD MD5 IMPL (No OpenSSL dependency) ---
                        auto inline_md5 = [](const std::string& msg) -> std::string {
                            unsigned int s[64] = {
                                7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
                                5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
                                4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
                                6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
                            };
                            unsigned int k[64] = {
                                0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
                                0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
                                0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
                                0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
                                0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
                                0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
                                0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
                                0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
                            };
                            
                            size_t orig_len = msg.length();
                            size_t new_len = (orig_len + 8) / 64 + 1;
                            new_len *= 64;
                            std::string pmsg(new_len, '\0');
                            std::copy(msg.begin(), msg.end(), pmsg.begin());
                            pmsg[orig_len] = (char)0x80;
                            
                            uint64_t bits_len = (uint64_t)orig_len * 8;
                            for(int i=0; i<8; i++) {
                                pmsg[new_len - 8 + i] = (char)(bits_len >> (i * 8));
                            }
                            
                            unsigned int h0 = 0x67452301, h1 = 0xefcdab89, h2 = 0x98badcfe, h3 = 0x10325476;
                            
                            for (size_t offset = 0; offset < new_len; offset += 64) {
                                unsigned int w[16];
                                for (int i = 0; i < 16; i++) {
                                    w[i] = (unsigned char)pmsg[offset + i*4] |
                                           ((unsigned char)pmsg[offset + i*4 + 1] << 8) |
                                           ((unsigned char)pmsg[offset + i*4 + 2] << 16) |
                                           ((unsigned char)pmsg[offset + i*4 + 3] << 24);
                                }
                                unsigned int a = h0, b = h1, c = h2, d = h3;
                                for (int i = 0; i < 64; i++) {
                                    unsigned int f, g;
                                    if (i < 16) { f = (b & c) | (~b & d); g = i; }
                                    else if (i < 32) { f = (d & b) | (~d & c); g = (5 * i + 1) % 16; }
                                    else if (i < 48) { f = b ^ c ^ d; g = (3 * i + 5) % 16; }
                                    else { f = c ^ (b | ~d); g = (7 * i) % 16; }
                                    unsigned int temp = d;
                                    d = c; c = b;
                                    unsigned int x = a + f + k[i] + w[g];
                                    b = b + ((x << s[i]) | (x >> (32 - s[i])));
                                    a = temp;
                                }
                                h0 += a; h1 += b; h2 += c; h3 += d;
                            }
                            
                            char res[33];
                            sprintf(res, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                                    h0 & 0xFF, (h0 >> 8) & 0xFF, (h0 >> 16) & 0xFF, (h0 >> 24) & 0xFF,
                                    h1 & 0xFF, (h1 >> 8) & 0xFF, (h1 >> 16) & 0xFF, (h1 >> 24) & 0xFF,
                                    h2 & 0xFF, (h2 >> 8) & 0xFF, (h2 >> 16) & 0xFF, (h2 >> 24) & 0xFF,
                                    h3 & 0xFF, (h3 >> 8) & 0xFF, (h3 >> 16) & 0xFF, (h3 >> 24) & 0xFF);
                            return std::string(res);
                        };

                        std::string calculatedToken = inline_md5(realString);
                        // ---------------------------------------------------------------

                        if (serverToken == calculatedToken) {
                            Connection.server_msg = "Login Success";
                        } else {
                            Connection.server_msg = "Invalid Token! Contact Admin";
                        }
                    } else {
                        Connection.server_msg = "Security Validation Failed";
                    }
                } else {
                    if (response.find("reason") != response.end() && !response["reason"].is_null()) {
                        Connection.server_msg = response["reason"].get<std::string>();
                    } else {
                        Connection.server_msg = "Login Failed! Contact Admin.";
                    }
                }
            } catch (...) {
                Connection.server_msg = "Parser Error";
            }
        } else {
            Connection.server_msg = "Connect Error";
        }
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    free(chunk.memory);
    env->ReleaseStringUTFChars(uKey, key);
    env->ReleaseStringUTFChars(uHwid, hwid);
    return env->NewStringUTF(Connection.server_msg.c_str());
}


// Add the missing showNativeAlertDialog function
extern "C" JNIEXPORT void JNICALL
Java_com_reaper_xxx_MainActivity_showNativeAlertDialog(JNIEnv* env, jobject thiz) {
    // Show a toast instead of a dialog
    jclass toastClass = env->FindClass("android/widget/Toast");
    if (toastClass == nullptr) return;
    
    jmethodID makeText = env->GetStaticMethodID(toastClass, "makeText", 
        "(Landroid/content/Context;Ljava/lang/CharSequence;I)Landroid/widget/Toast;");
    if (makeText == nullptr) return;
    
    jmethodID show = env->GetMethodID(toastClass, "show", "()V");
    if (show == nullptr) return;
    
    jstring message = env->NewStringUTF("Native Library Loaded!");
    
    jobject toast = env->CallStaticObjectMethod(toastClass, makeText, thiz, message, 0);
    if (toast != nullptr) {
        env->CallVoidMethod(toast, show);
    }
    
    env->DeleteLocalRef(message);
}

// Add JNI_OnLoad
extern "C" JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM* vm, void* reserved) {
    return JNI_VERSION_1_6;
}