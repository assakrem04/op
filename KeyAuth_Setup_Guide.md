# KeyAuth Free Web-Hosted License System Setup Guide

This guide explains how to set up your free KeyAuth web dashboard to generate and manage keys online without running any server on your PC.

---

## Step 1: Create Your Free KeyAuth Account

1. Go to [https://keyauth.cc](https://keyauth.cc) and click **Register**.
2. Sign in to your seller dashboard.
3. Click **Create Application**:
   - **Name**: Choose any application name (e.g. `MyLauncherApp`).
4. Once created, navigate to **App Settings** (or App Details) on the dashboard to copy your:
   - **App Name** (`KEYAUTH_NAME`)
   - **Owner ID** (`KEYAUTH_OWNER_ID`)
   - **Secret** (`KEYAUTH_SECRET`)

---

## Step 2: Put Credentials into Launcher.cpp

Open [Launcher.cpp](file:///C:/Users/suply/OneDrive%20-%20mfinances/Desktop/op/Launcher.cpp) and replace lines 13-15 with your credentials:

```cpp
string KEYAUTH_NAME     = "YourAppName";    // Replace with your App Name
string KEYAUTH_OWNER_ID = "YourOwnerID";   // Replace with your Owner ID
string KEYAUTH_SECRET   = "YourAppSecret"; // Replace with your Secret
```

Recompile `Launcher.exe` by running:
```powershell
g++ Launcher.cpp -o Launcher.exe -lwininet -lgdi32 -luser32 -mwindows
```

---

## Step 3: Generate Keys Online to Give to Users

1. On your [KeyAuth Dashboard](https://keyauth.cc), go to **Licenses** -> **Create License Key**.
2. Select your desired key options:
   - **Amount**: 1 or multiple keys.
   - **Expiry**: 1 Day, 7 Days, 30 Days, 365 Days, or Lifetime.
   - **Mask**: Key format pattern (default works great).
3. Click **Create License**.
4. Copy the generated license key and send it to your customer!

---

## Features included automatically:
- **HWID Locking**: The key locks to the customer's PC on first use.
- **Key Expiration**: Automatically expires after the chosen duration.
- **Pause & Ban Keys**: You can ban or pause keys anytime from your web dashboard.
- **Web-Hosted**: Completely free, hosted 24/7 on KeyAuth servers worldwide.
