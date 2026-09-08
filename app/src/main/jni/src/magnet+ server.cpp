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
#include <unordered_map>
#include <mutex>
#include <vector>
#include <algorithm>

// Global to prevent log spamming
uintptr_t last_failed_ptr = 0;

uintptr_t currentBestTargetObj = 0;
std::atomic<bool> keepAiming{false};
std::thread aimThread;
Vector3 currentBestTargetPos = Vector3::Zero();


namespace MAGNET {
    // Configuration
    std::atomic<bool> magnetEnabled{false};
    std::atomic<float> pullHeightOffset{1.5f};  
    
  
    std::thread magnetThread;
    std::atomic<bool> magnetRunning{false};
    std::mutex magnetMutex;
    std::unordered_map<uintptr_t, Vector3> originalPositions;
    std::atomic<uintptr_t> currentMagnetTarget{0};
    std::atomic<bool> wasFiringLastFrame{false};
    std::atomic<bool> isTargetPositionLocked{false};
    Vector3 lockedTargetPosition = Vector3::Zero();
    
    // Bone offsets (you need to fill these with correct values)
    const uintptr_t BONE_ROOT_OFFSET = 0x0;      
    const uintptr_t BONE_MATRIX_OFFSET = 0x0;      
    const uintptr_t FIRING_OFFSET = 0x0;          
    
    bool IsLocalFiring(uintptr_t localPlayer) {
        try {
            if (localPlayer == 0) return false;
            
      
            bool isFiring = Read<bool>(localPlayer + FIRING_OFFSET);
            return isFiring;
        } catch (...) {
            return false;
        }
    }
    
    Vector3 GetBonePosition(uintptr_t entityAddress) {
        try {
       
            uintptr_t bonePtr = Read<uintptr_t>(entityAddress + BONE_ROOT_OFFSET);
            if (bonePtr == 0) return Vector3::Zero();
            
            uintptr_t transformVal = Read<uintptr_t>(bonePtr + 0x8);
            if (transformVal == 0) return Vector3::Zero();
            
            uintptr_t transformObj = Read<uintptr_t>(transformVal + 0x8);
            if (transformObj == 0) return Vector3::Zero();
            
            uintptr_t matrixVal = Read<uintptr_t>(transformObj + 0x20);
            if (matrixVal == 0) return Vector3::Zero();
            
           
            Vector3 bonePos = Read<Vector3>(matrixVal + 0x80);
            return bonePos;
        } catch (...) {
            return Vector3::Zero();
        }
    }
    
    void SetBonePosition(uintptr_t entityAddress, const Vector3& newPosition) {
        try {
           
            uintptr_t bonePtr = Read<uintptr_t>(entityAddress + BONE_ROOT_OFFSET);
            if (bonePtr == 0) return;
            
            uintptr_t transformVal = Read<uintptr_t>(bonePtr + 0x8);
            if (transformVal == 0) return;
            
            uintptr_t transformObj = Read<uintptr_t>(transformVal + 0x8);
            if (transformObj == 0) return;
            
            uintptr_t matrixVal = Read<uintptr_t>(transformObj + 0x20);
            if (matrixVal == 0) return;
            
           
            Write<Vector3>(matrixVal + 0x80, newPosition);
        } catch (...) {
           
        }
    }
    
    uintptr_t FindClosestEntity(uintptr_t localPlayer, const std::vector<uintptr_t>& enemies, 
                               const D3DMatrix& cameraMatrix, float maxFOV) {
        if (enemies.empty() || localPlayer == 0) return 0;
        
        float centerX = g_screenWidth / 2.0f;
        float centerY = g_screenHeight / 2.0f;
        float fovRadiusSqr = maxFOV * maxFOV;
        
        uintptr_t closestEntity = 0;
        float closestDistanceSqr = FLT_MAX;
        
        for (uintptr_t enemy : enemies) {
            try {
               
                if (enemy == 0 || enemy == localPlayer) continue;
                
              
                bool isDead = Read<bool>(enemy + 0x74);  
                bool isKnocked = Read<bool>(enemy + 0x79); 
                if (isDead || isKnocked) continue;
                
            
                Vector3 headPos = GetBonePosition(enemy);
                if (headPos == Vector3::Zero()) continue;
                
           
                Vector3 screenPos = WorldToScreenPoint(cameraMatrix, headPos);
                if (screenPos.X < 0 || screenPos.Y < 0 || screenPos.X > g_screenWidth || screenPos.Y > g_screenHeight) {
                    continue;
                }
                
           
                float dx = screenPos.X - centerX;
                float dy = screenPos.Y - centerY;
                float distanceSqr = dx * dx + dy * dy;
                
              
                if (distanceSqr <= fovRadiusSqr && distanceSqr < closestDistanceSqr) {
                    closestEntity = enemy;
                    closestDistanceSqr = distanceSqr;
                }
            } catch (...) {
                continue;
            }
        }
        
        return closestEntity;
    }
    
    void LockTargetPosition(uintptr_t entityAddress, const Vector3& cameraPos, 
                           const Vector3& viewDirection) {
        std::lock_guard<std::mutex> lock(magnetMutex);
        
        try {
            currentMagnetTarget = entityAddress;
            
        
            Vector3 entityPos = GetBonePosition(entityAddress);
            if (entityPos == Vector3::Zero()) return;
            
            
            if (originalPositions.find(entityAddress) == originalPositions.end()) {
                originalPositions[entityAddress] = entityPos;
            }
            
        
            float realDistance3D = Vector3::Distance(cameraPos, entityPos);
            
          
            lockedTargetPosition = cameraPos + (viewDirection * realDistance3D);
            
        
            lockedTargetPosition.Y += pullHeightOffset;
            
            isTargetPositionLocked = true;
        } catch (...) {
          
        }
    }
    
    void ForceEntityToLockedPosition() {
        std::lock_guard<std::mutex> lock(magnetMutex);
        
        if (!isTargetPositionLocked || currentMagnetTarget == 0) return;
        
        try {
            SetBonePosition(currentMagnetTarget, lockedTargetPosition);
        } catch (...) {
            // Error handling
        }
    }
    
    void RestoreOriginalPosition() {
        std::lock_guard<std::mutex> lock(magnetMutex);
        
        if (currentMagnetTarget != 0) {
            auto it = originalPositions.find(currentMagnetTarget);
            if (it != originalPositions.end()) {
                SetBonePosition(currentMagnetTarget, it->second);
                originalPositions.erase(it);
            }
            
            currentMagnetTarget = 0;
        }
        
        isTargetPositionLocked = false;
    }
    
    void Work() {
        if (magnetRunning) return;
        
        magnetRunning = true;
        magnetThread = std::thread([]() {
            while (magnetRunning) {
                // Small delay to prevent CPU overuse
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                
                if (!magnetEnabled) {
                    if (isTargetPositionLocked) {
                        RestoreOriginalPosition();
                    }
                    wasFiringLastFrame = false;
                    continue;
                }
                
                // Get local player (you need to implement this based on your existing code)
                uintptr_t localPlayer = 0; // GetLocalPlayer();
                if (localPlayer == 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                
                // Check firing state
                bool isCurrentlyFiring = IsLocalFiring(localPlayer);
                
                if (isCurrentlyFiring) {
                    if (!isTargetPositionLocked) {
                        // Get camera position and view direction
                        Vector3 cameraPos = Vector3::Zero(); // GetCameraPosition();
                        D3DMatrix cameraMatrix; // GetCameraMatrix();
                        Vector3 viewDirection = Vector3::Normalize(
                            Vector3(cameraMatrix.M13, cameraMatrix.M23, cameraMatrix.M33));
                        
                        // Get list of enemies (you need to implement this based on your existing code)
                        std::vector<uintptr_t> enemies; // GetEnemyList();
                        
                        // Find closest entity
                        uintptr_t closestEntity = FindClosestEntity(localPlayer, enemies, 
                                                                   cameraMatrix, 450.0f);
                        
                        if (closestEntity != 0) {
                            LockTargetPosition(closestEntity, cameraPos, viewDirection);
                        }
                    }
                    
                    if (isTargetPositionLocked) {
                        ForceEntityToLockedPosition();
                    }
                    
                    wasFiringLastFrame = true;
                } else if (wasFiringLastFrame) {
                    RestoreOriginalPosition();
                    wasFiringLastFrame = false;
                }
            }
            
            // Cleanup when thread stops
            RestoreOriginalPosition();
        });
    }
    
    void Stop() {
        if (!magnetRunning) return;
        
        magnetRunning = false;
        if (magnetThread.joinable()) {
            magnetThread.join();
        }
        
        RestoreOriginalPosition();
    }
    
    void SetEnabled(bool enabled) {
        magnetEnabled = enabled;
        if (!enabled) {
            RestoreOriginalPosition();
        }
    }
    
    void SetHeightOffset(float offset) {
        pullHeightOffset = offset;
    }
}

// ============================================================================
// EXISTING FUNCTIONS (modified to support magnet)
// ============================================================================

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
            Vector3 startPos = Read<Vector3>(aimingInfo + 0x4c);
            Vector3 dir = currentBestTargetPos - startPos;  
            Write<Vector3>(aimingInfo + 0x40, dir);
        }
    }  
}

void CreateDataList(Response& SendResponse) {
    if (Actived.activar) {
        float bestDistance = 99999;
        bool hasValidTarget = false;
        Vector3 bestTargetPos = Vector3::Zero();
        uintptr_t bestTargetObj = 0;
        SendResponse.PlayerCount = 0;
        
        // Collect enemy list for magnet hack
        std::vector<uintptr_t> enemyListForMagnet;
        
        uintptr_t gameFacadeOffset = 0xC2132B8; 
        auto GameFacade_c = Read<uintptr_t>(libAddress + gameFacadeOffset);
        
        if (GameFacade_c) {
            auto P2 = Read<uintptr_t>(GameFacade_c + 0xB8);
            if(P2) {
                auto BaseGame = Read<uintptr_t>(P2);
                if (BaseGame) {
                    auto m_Match = Read<uintptr_t>(BaseGame + 0x90);
                    if (m_Match) {
                        auto MatchIsRunning = Read<int>(m_Match + 0xA4);
                        if(MatchIsRunning != 4 && MatchIsRunning != 0 && MatchIsRunning == 1) {
                            auto localPlayer = Read<uintptr_t>(m_Match + 0xb0);
                            if (localPlayer != 0) {
                                auto FollowCamera = Read<uintptr_t>(localPlayer + 0x5b0);
                                if (FollowCamera != 0) {
                                    auto Camera = Read<uintptr_t>(FollowCamera + 0x28);
                                    if (Camera != 0) {
                                        auto IntPtrCam = Read<uintptr_t>(Camera + 0x10);
                                        if (IntPtrCam != 0) {
                                            uintptr_t matrixOffset = string2Offset(WRAPPER_MARCO("0xD8"));
                                            auto matrix = Read<D3DMatrix>(IntPtrCam + matrixOffset);
                                            auto dictionary = Read<MonoDictionary *>(BaseGame + 0xc0);
                                            
                                            if (dictionary) {
                                                int numValues = dictionary->getNumValues();
                                                auto LocalPosition = GetNodePosition(GetPlayerHeadTF(localPlayer));
                                                
                                                // Get camera info for magnet hack
                                                Vector3 cameraPos = Vector3::Zero(); // Get camera position
                                                Vector3 viewDirection = Vector3::Normalize(
                                                    Vector3(matrix.M13, matrix.M23, matrix.M33));
                                                
                                                for (int x = 0; x < numValues; x++) {
                                                    auto enemyList = Read<uintptr_t>(dictionary->getValues() + 0x8 * x);
                                                    if (enemyList != 0) {
                                                        auto isClientBot = Read<bool>(enemyList + 0x3d8);
                                                        if (isClientBot == 1 || isClientBot == 0) {
                                                            auto AvatarManager = Read<uintptr_t>(enemyList + 0x690);
                                                            if (AvatarManager != 0 && enemyList != localPlayer) {
                                                                auto UmaAvatarSimple = Read<uintptr_t>(AvatarManager + 0x118);
                                                                if (UmaAvatarSimple != 0) {
                                                                    auto IsVisible = Read<bool>(UmaAvatarSimple + 0xD8);
                                                                    if (IsVisible == 1) {
                                                                        auto UmaData = Read<uintptr_t>(UmaAvatarSimple + 0x20);
                                                                        if (UmaData != 0) {
                                                                            auto HHCBNAPCKHF = Read<uintptr_t>(enemyList + 0x1ee0);
                                                                            if (HHCBNAPCKHF != 0) {
                                                                                bool isDieing = false;
                                                                                auto maybeDead = Read<uintptr_t>(HHCBNAPCKHF + 0x18);
                                                                                if (maybeDead && Read<int>(maybeDead + 0x10) == 8) isDieing = true;
                                                                                
                                                                                if (Read<bool>(UmaData + 0x79) != 0) continue;
                                                                                if (Read<bool>(enemyList + 0x74) != 0) continue;
                                                                                
                                                                                int Health = 0;
                                                                                auto GetCurHP = Read<uintptr_t>(enemyList + 0x68);
                                                                                if (GetCurHP != 0) {
                                                                                    auto GetCur = Read<uintptr_t>(GetCurHP + 0x10);
                                                                                    if (GetCur != 0) {
                                                                                        auto HP = Read<uintptr_t>(GetCur + 0x20);
                                                                                        if (HP != 0) Health = (int)Read<short>(HP + 0x18);
                                                                                    }
                                                                                }
                                                                                
                                                                                auto HedColider = Read<uintptr_t>(enemyList + 0x658);
                                                                                if (pPlayer.aimbotlock) Write(enemyList + 0x78, HedColider);
                                                                                
                                                                                auto weapon = Read<uintptr_t>(localPlayer + 0x4c8);
                                                                                if (pPlayer.aimbotlock && weapon != 0) {
                                                                                 float newAimLock = 0.0f;
                                                                                 Write(weapon + 0x4dc, newAimLock);
                                                                                }
                                                                                
                                                                                auto EnemyPosition = GetNodePosition(GetPlayerHeadTF(enemyList));
                                                                                auto EnemyPositionPe = GetNodePosition(GetPlayerPeTF(enemyList));
                                                                                if (LocalPosition == Vector3::Zero() || EnemyPosition == Vector3::Zero()) continue;
                                                                                
                                                                                float distanceToEnemy = Vector3::Distance(LocalPosition, EnemyPosition);
                                                                                auto screenPos = WorldToScreenPoint(matrix, EnemyPosition);
                                                                                float fovRadius = (pPlayer.AimFov / 100.0f) * (g_screenHeight / 2.0f);
                                                                                
                                                                                // Add to magnet enemy list
                                                                                enemyListForMagnet.push_back(enemyList);
                                                                                
                                                                                if (pPlayer.silen4a && isWithinFOV(screenPos, fovRadius) && distanceToEnemy <= 100.0f) {
                                                                                    if (distanceToEnemy < bestDistance) {
                                                                                        bestDistance = distanceToEnemy;
                                                                                        bestTargetPos = EnemyPosition;
                                                                                        bestTargetObj = enemyList;
                                                                                        hasValidTarget = true;
                                                                                    }
                                                                                }
                                                                                
                                                                                auto LocationHeadBox = WorldToScreenPoint(matrix, EnemyPosition);
                                                                                auto LocationToeBox = WorldToScreenPoint(matrix, EnemyPositionPe);
                                                                                if (LocationHeadBox.X < -0 || LocationToeBox.X < -0) continue;
                                                                                
                                                                                auto *data = &SendResponse.Players[SendResponse.PlayerCount];
                                                                                
                                                                                memset(data->PlayerName, 0, 64);
                                                                                if (isClientBot) {
                                                                                    strcpy(data->PlayerName, "Enemy");
                                                                                } else {
                                                                                    uintptr_t NamePtr = Read<uintptr_t>(enemyList + 0x3d0); 
                                                                                    if (NamePtr > 0x10000000) {
                                                                                        getUTF8(data->PlayerName, NamePtr);
                                                                                    }
                                                                                    if (data->PlayerName[0] == '\0') strcpy(data->PlayerName, "Enemy");
                                                                                }

                                                                                data->Head = LocationHeadBox;
                                                                                data->Toe = LocationToeBox;
                                                                                data->IsCaido = isDieing;
                                                                                data->Distancia = distanceToEnemy;
                                                                                data->Name = isClientBot;
                                                                                data->Health = Health;
                                                                                
                                                                                ++SendResponse.PlayerCount;
                                                                                if (SendResponse.PlayerCount == maxplayerCount) break;
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                                
                                                // Handle magnet hack
                                                if (MAGNET::magnetEnabled) {
                                                    // Find closest entity for magnet
                                                    uintptr_t closestEntity = MAGNET::FindClosestEntity(localPlayer, 
                                                                                                      enemyListForMagnet, 
                                                                                                      matrix, 450.0f);
                                                    
                                                    // Check if we should lock target
                                                    bool isFiring = MAGNET::IsLocalFiring(localPlayer);
                                                    if (isFiring && closestEntity != 0) {
                                                        if (!MAGNET::isTargetPositionLocked) {
                                                            MAGNET::LockTargetPosition(closestEntity, cameraPos, viewDirection);
                                                        }
                                                    }
                                                }
                                                
                                                if (pPlayer.silen4a && hasValidTarget) {
                                                    currentBestTargetPos = bestTargetPos;
                                                    currentBestTargetObj = bestTargetObj;
                                                    if (!keepAiming) {
                                                        keepAiming = true;
                                                        if (aimThread.joinable()) aimThread.join();  
                                                        aimThread = std::thread([&]() { ContinuousAim(localPlayer); });  
                                                    }
                                                } else if (keepAiming) {
                                                    keepAiming = false;
                                                    if (aimThread.joinable()) aimThread.join();  
                                                    currentBestTargetPos = Vector3::Zero();  
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
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

            // Start magnet thread
            MAGNET::Work();
            
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
                }
                // Magnet hack controls
                else if(request.Mode == 59){  // Enable/Disable magnet
                    MAGNET::SetEnabled(request.m_IsOn);
                    SendResponse.Success = true;
                }
                else if(request.Mode == 60){  // Set magnet height offset
                    MAGNET::SetHeightOffset((float)request.value);
                    SendResponse.Success = true;
                }
                
                server.send((void*)& SendResponse, sizeof(SendResponse));
            }
        }
    }
    
    MAGNET::Stop();
    if (aimThread.joinable()) {
        aimThread.join();
    }
    
    return 0;
}