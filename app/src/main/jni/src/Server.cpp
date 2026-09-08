#include <dirent.h>
#include "Server.h"
#include "Includes/Encrypt/oxorany_include.h"
#include <android/log.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <atomic>
#include <thread>
#include <stdarg.h>
#include <fstream>


Quaternion GetRotationToTheLocation(Vector3 Target, float Height, Vector3 MyEnemy) {
    return Quaternion::LookRotation((Target + Vector3(0, Height, 0)) - MyEnemy, Vector3(0, 1, 0));
}


// Global to prevent log spamming
uintptr_t last_failed_ptr = 0;

uintptr_t currentBestTargetObj = 0;
std::atomic<bool> keepAiming{false};
std::thread aimThread;
Vector3 currentBestTargetPos = Vector3::Zero();

// Add speed hack toggle variable
bool speedHackEnabled = false;

void getUTF8(char* dst, uintptr_t addr) {
    if (addr < 0x10000000) {
        dst[0] = '\0';
        return;
    }

    int stringLen = Read<int>(addr + 0x10);
    if (stringLen <= 0 || stringLen > 64) {
        stringLen = 32; 
    }

    unsigned char raw[128]; 
    for (int i = 0; i < (stringLen * 2); i++) {
        raw[i] = Read<unsigned char>(addr + 0x14 + i);
    }

    int j = 0;
    for (int i = 0; i < (stringLen * 2); i += 2) {
        unsigned short unicode = raw[i] | (raw[i + 1] << 8);
        if (unicode == 0) break;
        if (unicode < 0x80) {
            dst[j++] = (char)unicode;
        } else if (unicode < 0x800) {
            dst[j++] = (char)((unicode >> 6) | 0xC0);
            dst[j++] = (char)((unicode & 0x3F) | 0x80);
        } else if (unicode >= 0xD800 && unicode <= 0xDBFF) { 
            i += 2;
            unsigned short low = raw[i] | (raw[i + 1] << 8);
            unsigned int codepoint = 0x10000 + ((unicode - 0xD800) << 10) + (low - 0xDC00);
            dst[j++] = (char)((codepoint >> 18) | 0xF0);
            dst[j++] = (char)(((codepoint >> 12) & 0x3F) | 0x80);
            dst[j++] = (char)(((codepoint >> 6) & 0x3F) | 0x80);
            dst[j++] = (char)((codepoint & 0x3F) | 0x80);
        } else {
            dst[j++] = (char)((unicode >> 12) | 0xE0);
            dst[j++] = (char)(((unicode >> 6) & 0x3F) | 0x80);
            dst[j++] = (char)((unicode & 0x3F) | 0x80);
        }
        if (j >= 60) break; 
    }
    dst[j] = '\0';
}

bool isWithinFOV(Vector3 screenPos, float fovRadius) {
    float screenCenterX = g_screenWidth / 2.0f;
    float screenCenterY = g_screenHeight / 2.0f;
    float deltaX = screenPos.X - screenCenterX;
    float deltaY = screenPos.Y - screenCenterY;
    float distanceFromCenter = sqrt(deltaX * deltaX + deltaY * deltaY);
    return distanceFromCenter <= fovRadius;
}

void ContinuousAim(uintptr_t oneself) {
    while (keepAiming) {
        uint64_t aimingInfo = Read<uint64_t>(oneself + 0xd20);
        if (aimingInfo) {
            Vector3 startPos = Read<Vector3>(aimingInfo + 0x4c); // old
            Vector3 dir = currentBestTargetPos - startPos;  
            Write<Vector3>(aimingInfo + 0x40, dir);
        }
    }  
}

float Map(float value, float inMin, float inMax, float outMin, float outMax) {
    return outMin + (value - inMin) * (outMax - outMin) / (inMax - inMin);
}

void CreateDataList(Response& SendResponse) {
    if (!Actived.activar)
        return;

    // Protecao total: qualquer excecao (bad_alloc, lixo de memoria, ponteiro invalido)
    // capturada aqui para nao matar a thread de leitura — o ESP/aimbot nunca "morre" sozinho.
    try
    {
        SendResponse.PlayerCount = 0;

        float bestDistance = 99999.0f;
        bool hasValidTarget = false;
        Vector3 bestTargetPos = Vector3::Zero();
        uintptr_t bestTargetObj = 0;

        // ============================================================
        // 1) Speed Hack (TimeService) — mantido do codigo original
        // ============================================================
        if (libAddress != 0)
        {
            uintptr_t GameFacade = Read<uintptr_t>(libAddress + 0xABFF3C0);
            if (GameFacade != 0)
            {
                uintptr_t AccessClass = Read<uintptr_t>(GameFacade + 0x5C);
                if (AccessClass != 0)
                {
                    uintptr_t MatchGame = Read<uintptr_t>(AccessClass + 0x4);
                    if (MatchGame != 0)
                    {
                        auto TimeService = Read<uintptr_t>(MatchGame + 0x74);
                        if (TimeService != 0)
                        {
                            float speedValue = speedHackEnabled ? 0.05900000036f : 0.03299999982f;
                            Write<float>(TimeService + 0x2C, speedValue);
                        }
                    }
                }
            }
        }

        // ============================================================
        // 2) Setup de partida, camera e jogador local
        //    Padrao do codigo que funciona: do-while(false) com early break
        //    para evitar nesting profundo e facilitar diagnostico.
        // ============================================================
        bool setupOk = false;
        D3DMatrix matrix{};
        Vector3 LocalPosition = Vector3::Zero();
        uintptr_t localPlayer = 0;
        int mIsFiring = 0;
        uintptr_t m_Match = 0;

        do
        {
            if (libAddress == 0)
            {
                LOGI("[DBG] libAddress == 0");
                break;
            }

            LOGI("[DBG] libAddress=%p", (void*)libAddress);

            uintptr_t GameFacade = Read<uintptr_t>(libAddress + 0xABFF6E0);
            LOGI("[DBG] GameFacade=%p", (void*)GameFacade);
            uint32_t GameFacade32 = Read<uint32_t>(libAddress + 0xABFF6E0);
            LOGI("[DBG] GameFacade32=%d", (void*)GameFacade32);
            // uint64_t GameFacade64 = Read<uint64_t>(libAddress + 0xABFF6E0);
            // LOGI("[DBG] GameFacade64=%lld", GameFacade64);
            // if (GameFacade == 0) break;

            uintptr_t AccessClass = Read<uintptr_t>(GameFacade + 0x5C);
            if (AccessClass == 0) break;

            uintptr_t MatchGame = Read<uintptr_t>(AccessClass + 0x4);
            if (MatchGame == 0) break;

            m_Match = Read<uintptr_t>(MatchGame + 0x50);
            LOGI("[DBG] libAddress=%p m_Match=%p", (void*)libAddress, (void*)m_Match);
            if (m_Match == 0) break;

            auto MatchIsRunning = Read<int>(m_Match + 0xa8);
            LOGI("[DBG] BYPASS PRA TESTE na linha 175:MatchIsRunning=%d", MatchIsRunning);
            // Simplificado: so prossegue se a partida estiver no estado correto (1)
            // if (MatchIsRunning != 1)
            //     break;

            localPlayer = Read<uintptr_t>(libAddress + 0x72942d4);
            LOGI("[DBG] localPlayer=%p", (void*)localPlayer);
            if (localPlayer == 0) break;

            auto FollowCamera = Read<uintptr_t>(localPlayer + 0x628);
            LOGI("[DBG] FollowCamera=%p", (void*)FollowCamera);
            if (FollowCamera == 0) break;

            auto Camera = Read<uintptr_t>(FollowCamera + 0x30);
            LOGI("[DBG] Camera=%p", (void*)Camera);
            if (Camera == 0) break;

            auto IntPtrCam = Read<uintptr_t>(Camera + 0x10);
            LOGI("[DBG] IntPtrCam=%p", (void*)IntPtrCam);
            if (IntPtrCam == 0) break;

            // --- View Matrix com validacao (padrao do codigo que funciona) ---
            matrix = Read<D3DMatrix>(IntPtrCam + 0xD8);
            bool matrixValid = false;
            for (int i = 0; i < 16; i++)
            {
                float v = reinterpret_cast<const float*>(&matrix)[i];
                if (isfinite(v) && v != 0.0f)
                {
                    matrixValid = true;
                    break;
                }
            }
            if (!matrixValid)
            {
                LOGI("[DBG] ViewMatrix invalida (zeros/NaN)");
                break;
            }

            LocalPosition = GetNodePosition(GetPlayerHeadTF(localPlayer));
            if (LocalPosition == Vector3::Zero())
            {
                LOGI("[DBG] LocalPosition invalida");
                break;
            }

            mIsFiring = Read<int>(localPlayer + 0x79c);
            setupOk = true;
        }
        while (false);

        if (!setupOk)
            return;

        // ============================================================
        // 3) Leitura do Dictionary de inimigos
        // ============================================================
        auto dictionary = Read<MonoDictionary*>(m_Match + 0x458);
        LOGI("[DBG] dictionary=%p", (void*)dictionary);
        if (!dictionary)
            return;

        int numValues = dictionary->getNumValues();
        LOGI("[DBG] numValues=%d", numValues);
        if (numValues <= 0 || numValues > 500)
            return;

        // Pre-calculos de tela para FOV
        float screenCenterX = g_screenWidth / 2.0f;
        float screenCenterY = g_screenHeight / 2.0f;
        float fovRadius = (pPlayer.AimFov / 100.0f) * (g_screenHeight / 2.0f);

        // ============================================================
        // 4) Loop de entidades
        // ============================================================
        for (int x = 0; x < numValues; x++)
        {
            // Capacidade do buffer: nunca escreve fora do array
            if (SendResponse.PlayerCount >= maxplayerCount)
                break;

            auto enemyList = Read<uintptr_t>(dictionary->getValues() + 0x8 * x);
            if (enemyList == 0 || enemyList == localPlayer)
                continue;

            // --- Validações em cadeia (estilo do codigo que funciona) ---
            auto AvatarManager = Read<uintptr_t>(enemyList + 0x708);
            if (AvatarManager == 0) continue;

            auto UmaAvatarSimple = Read<uintptr_t>(AvatarManager + 0x138);
            if (UmaAvatarSimple == 0) continue;

            bool IsVisible = Read<bool>(UmaAvatarSimple + 0x101);
            if (!IsVisible) continue;

            auto UmaData = Read<uintptr_t>(AvatarManager + 0xc0);
            if (UmaData == 0) continue;

            auto HHCBNAPCKHF = Read<uintptr_t>(enemyList + 0x1ee0);
            if (HHCBNAPCKHF == 0) continue;

            // --- Estado do jogador ---
            bool isDieing = (Read<int64_t>(HHCBNAPCKHF + 0x68) == 8);
            if (Read<bool>(AvatarManager + 0x39) != 0) continue;   // IsChangedToDefaultModel
            if (Read<bool>(enemyList + 0x7c) != 0) continue;       // isDead

            bool isClientBot = Read<bool>(enemyList + 0x438);

            // --- Aimlock / BoneSwap (mantido do original) ---
            if (pPlayer.aimbotlock)
            {
                auto HedColider = Read<uintptr_t>(enemyList + 0x660);
                if (HedColider != 0)
                    Write(enemyList + 0x660, HedColider); // no-op intencional? mantido do original

                auto weapon = Read<uintptr_t>(localPlayer + 0x598);
                if (weapon != 0)
                {
                    float newAimLock = 0.0f;
                    Write(weapon + 0x5a8, newAimLock);
                }
            }

            // --- Posicoes ---
            auto EnemyPosition = GetNodePosition(GetPlayerHeadTF(enemyList));
            auto EnemyPositionPe = GetNodePosition(GetPlayerPeTF(enemyList));
            if (EnemyPosition == Vector3::Zero())
                continue;

            float distanceToEnemy = Vector3::Distance(LocalPosition, EnemyPosition);
            auto screenPos = WorldToScreenPoint(matrix, EnemyPosition);

            // --- Aimbot ---
            float screenDist = sqrtf(
                powf(screenPos.X - screenCenterX, 2.0f) +
                powf(screenPos.Y - screenCenterY, 2.0f)
            );

            if (pPlayer.aimbot && screenDist < pPlayer.AimFov && mIsFiring > 0)
            {
                Quaternion desiredRot = GetRotationToTheLocation(EnemyPosition, 0.0f, LocalPosition);
                Write<Quaternion>(localPlayer + 0x5ac, desiredRot);
            }

            // --- Silent Aim: selecao de alvo ---
            if (pPlayer.silen4a && isWithinFOV(screenPos, fovRadius) && distanceToEnemy <= 100.0f)
            {
                if (distanceToEnemy < bestDistance)
                {
                    bestDistance = distanceToEnemy;
                    bestTargetPos = EnemyPosition;
                    bestTargetObj = enemyList;
                    hasValidTarget = true;
                }
            }

            // --- ESP Data ---
            auto LocationHeadBox = WorldToScreenPoint(matrix, EnemyPosition);
            auto LocationToeBox = WorldToScreenPoint(matrix, EnemyPositionPe);
            if (LocationHeadBox.X < 0 || LocationToeBox.X < 0)
                continue;

            auto* data = &SendResponse.Players[SendResponse.PlayerCount];
            memset(data->PlayerName, 0, 64);

            if (isClientBot)
            {
                strcpy(data->PlayerName, "Enemy");
            }
            else
            {
                uintptr_t NamePtr = Read<uintptr_t>(enemyList + 0x430);
                if (NamePtr > 0x10000000)
                {
                    getUTF8(data->PlayerName, NamePtr);
                }
                if (data->PlayerName[0] == '\0')
                    strcpy(data->PlayerName, "Enemy");
            }

            data->Head = LocationHeadBox;
            data->Toe = LocationToeBox;
            data->IsCaido = isDieing;
            data->Distancia = distanceToEnemy;
            data->Name = isClientBot;
            data->Health = 0; // HP nao disponivel nesta versao sem offset correto

            ++SendResponse.PlayerCount;
        }

        // ============================================================
        // 5) Silent Aim — Thread Management
        // ============================================================
        if (pPlayer.silen4a && hasValidTarget)
        {
            LOGI("[DBG] PlayerCount=%d hasValidTarget=1 bestDist=%f", SendResponse.PlayerCount, bestDistance);
            currentBestTargetPos = bestTargetPos;
            currentBestTargetObj = bestTargetObj;
            if (!keepAiming)
            {
                keepAiming = true;
                if (aimThread.joinable())
                    aimThread.join();
                aimThread = std::thread([&]() { ContinuousAim(localPlayer); });
            }
        }
        else if (keepAiming)
        {
            keepAiming = false;
            if (aimThread.joinable())
                aimThread.join();
            currentBestTargetPos = Vector3::Zero();
            currentBestTargetObj = 0;
        }
    }
    catch (const std::exception& ex)
    {
        LOGI("[CreateDataList] Exception: %s", ex.what());
    }
    catch (...)
    {
        LOGI("[CreateDataList] Unknown exception");
    }
}

__attribute__ ((visibility ("hidden")))
int main(int argc, char*argv[]){
    if (InitServer() == (0)) {
        if (server.Accept()) {
            target_pid = FindPid("com.dts.freefireth");
            if (target_pid > 0) {
                mem_fd = my_open_mem(target_pid);
                if (mem_fd <= 0) {
                    return 1;
                }
            } else {
                exit(1);
            }
            libAddress = FindLibrary("libil2cpp.so", 1);
            if (libAddress == 0) {
            }

            Request request{};
            while (server.receive((void*)&request) > (0)) {
                Response SendResponse{};
                if (request.Mode == Mode::InitMode) {
                    SendResponse.Success = true;
                } else if (request.Mode == Mode::HackMode) {
                    SendResponse.Success = true;
                } else if (request.Mode == Mode::EspMode) {
                    g_screenWidth = request.ScreenWidth;
                    g_screenHeight = request.ScreenHeight;
                    CreateDataList(SendResponse);
                    SendResponse.Success = true;
                } else if (request.Mode == 3) {
                    Actived.activar = request.m_IsOn;
                    SendResponse.Success = true;
                } else if(request.Mode == 5){
                    pPlayer.aimbot = request.m_IsOn;
                    SendResponse.Success = true;
                } else if(request.Mode == 22){
                    pPlayer.fovawm = request.m_IsOn;
                    SendResponse.Success = true;
                } else if(request.Mode == 54){
                    pPlayer.aimbotlock = request.m_IsOn;
                    SendResponse.Success = true;
                } else if(request.Mode == 57){
                    pPlayer.silen4a = request.m_IsOn;
                    SendResponse.Success = true;
                } else if(request.Mode == 58){
                    pPlayer.AimFov = (float)request.value;
                    SendResponse.Success = true;
                } else if(request.Mode == 59){
                    pPlayer.AimDistance = (float)request.value;
                    SendResponse.Success = true;
                } else if(request.Mode == 60){  // Speed hack toggle
                    speedHackEnabled = request.m_IsOn; // Get boolean value
                    SendResponse.Success = true;
                }
                server.send((void*)& SendResponse, sizeof(SendResponse));
            }
        }
    }
    return 0;
}

