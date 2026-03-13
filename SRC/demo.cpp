// #include <iostream>
// #include <vector>
// #include <string>
// #include <Eigen/Dense> // Used for matrix operations

// #include "ndicapi.h"

// // Macro to cast string literals to char* for the legacy NDI C API
// #define NDI_C(s) const_cast<char*>(s)

// /**
//  * Structure to hold the parsed data from NDI
//  */
// struct NDITransform {
//     double q[4];    // Quaternion: [qw, qx, qy, qz]
//     double t[3];    // Translation: [x, y, z] in mm
//     double error;   // RMS fitting error
//     bool visible;   // Is the tool currently tracked?
// };

// /**
//  * Step 5: Convert the parsed NDI data into an Eigen 4x4 Transformation Matrix
//  */
// Eigen::Matrix4d CreateTransformMatrix(const NDITransform& data) {
//     Eigen::Matrix4d matrix = Eigen::Matrix4d::Identity();

//     if (data.visible) {
//         // Construct rotation part from Quaternion (w, x, y, z)
//         Eigen::Quaterniond q(data.q[0], data.q[1], data.q[2], data.q[3]);
//         matrix.block<3, 3>(0, 0) = q.toRotationMatrix();

//         // Construct translation part (Position)
//         matrix(0, 3) = data.t[0];
//         matrix(1, 3) = data.t[1];
//         matrix(2, 3) = data.t[2];
//     }
//     return matrix;
// }

// /**
//  * Helper: Parse the GX ASCII reply string into our struct
//  */
// bool ParseNDIGXReply(const char* reply, NDITransform& outData) {
//     if (!reply || strlen(reply) < 50) return false;
//     // Check status: "0001" means valid 6D data
//     if (strncmp(reply + 4, "0001", 4) != 0) {
//         outData.visible = false;
//         return false;
//     }
//     int iQ0, iQx, iQy, iQz, iTx, iTy, iTz, iErr;
//     if (sscanf(reply + 8, "%6d%6d%6d%6d%7d%7d%7d%5d", 
//                &iQ0, &iQx, &iQy, &iQz, &iTx, &iTy, &iTz, &iErr) == 8) {
//         outData.q[0] = iQ0 / 10000.0;
//         outData.q[1] = iQx / 10000.0;
//         outData.q[2] = iQy / 10000.0;
//         outData.q[3] = iQz / 10000.0;
//         outData.t[0] = iTx / 100.0;
//         outData.t[1] = iTy / 100.0;
//         outData.t[2] = iTz / 100.0;
//         outData.error = iErr / 10000.0;
//         outData.visible = true;
//         return true;
//     }
//     return false;
// }

// int main() {
//     // 1. Connection (Assuming COM3 for this example)
//     ndicapi* device = nullptr;
//     const char* autoPort = nullptr;
//     for (int i = 0; i < 10; ++i) {
//         // Get the system name for the i-th port (e.g., "COM1", "COM2"...)
//         const char* name = ndiSerialDeviceName(i); 
        
//         // Test if there's an NDI device on this port
//         if (ndiSerialProbe(NDI_C(name), false) == NDI_OKAY) {
//             autoPort = name;
//             std::cout << "Found NDI device on: " << autoPort << std::endl;
//             break;
//         }
//     }

//     if (autoPort) {
//         device = ndiOpenSerial(NDI_C(autoPort));
//     }
//     if (!device) return -1;

//     // 2. Initialize Hardware
//     ndiCommand(device, NDI_C("INIT:"));

//     // 3. Step-by-Step Tool Setup
//     // -----------------------------------------------------------------
    
//     // A. Request a 'Port Handle' (Logical ID) for a new tool
//     // "PHSR:01" asks for handles that need to be initialized
//     int toolHandle = -1;
//     const char* phsrReply = ndiCommand(device, NDI_C("PHSR:01"));
//     if (phsrReply && strlen(phsrReply) >= 2) {
//         unsigned int uHandle = 0;
//         sscanf(phsrReply, "%02X", &uHandle); // Parse 2-digit Hex
//         toolHandle = static_cast<int>(uHandle);
//     }

//     if (toolHandle <= 0) {
//         std::cerr << "No free handles available." << std::endl;
//         return -1;
//     }

//     // B. Load the ROM file into the assigned Handle
//     // Replace with your actual .rom file path
//     const char* romPath = "C:/NDI/Tools/Pointer.rom"; 
//     if (ndiPVWRFromFile(device, toolHandle, NDI_C(romPath)) != NDI_OKAY) {
//         std::cerr << "Failed to load ROM file." << std::endl;
//         return -1;
//     }

//     // C. Initialize (PINIT) and Enable (PENA) the tool
//     // 'D' means Dynamic - for tracking moving tools
//     ndiCommand(device, NDI_C("PINIT:%02X"), toolHandle);
//     ndiCommand(device, NDI_C("PENA:%02X%c"), toolHandle, 'D');

//     // D. Start Tracking (The camera's infrared illuminators turn on)
//     ndiCommand(device, NDI_C("TSTART:"));

//     // -----------------------------------------------------------------
//     // 4. Tracking Loop (Get Matrix)
//     // -----------------------------------------------------------------
    
//     while (true) {
//         // Send GX command to get data for our specific handle
//         char gxCmd[16];
//         sprintf(gxCmd, "GX:%02X", toolHandle);
//         const char* reply = ndiCommand(device, gxCmd);

//         NDITransform transformData;
//         if (ParseNDIGXReply(reply, transformData)) {
//             // Success: Tool is visible
//             Eigen::Matrix4d mat = CreateTransformMatrix(transformData);

//             // Print the Matrix
//             std::cout << "\n--- Tool Matrix (Relative to Camera) ---" << std::endl;
//             std::cout << mat << std::endl;
//             std::cout << "RMS Error: " << transformData.error << " mm" << std::endl;
//         } else {
//             // Tool is out of view or blocked
//             std::cout << "Tool is not visible to camera." << std::endl;
//         }

//         std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30Hz
//     }

//     // 5. Cleanup
//     ndiCommand(device, NDI_C("TSTOP:"));
//     ndiCloseSerial(device);
//     return 0;
// }