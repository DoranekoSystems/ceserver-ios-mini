# ceserver-ios-mini

Limited functionality ceserver for iOS.

# Usage

Jailbreaking of iphone is required.  
Place your PC and iphone in the same network.  
Place ceserver and Entitlements.plist in /usr/bin.

Connect to the iphone via ssh.

```
cd /usr/bin
chmod a+x ceserver
ldid -SEntitlements.plist ceserver
./ceserver
```

Set native_ceserver_ip in frida-ceserver's config.json to the iphone's ip address.

Cheat Engine <-> frida-ceserver <-> ceserver

# Build

`./build.sh`
