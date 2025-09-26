// Adds the file systems

#include "debug_port.h"
void addFileSystems(void) {  // not used!! 
  // set configTzTime() in setup() to get valid file dates. Otherwise they are kaputt[tm].

  // This adds the Storage into the Filemanager. You have to at least call one of those.
  // If you don't, begin() will fail. Because a Filemanager without files is useless.

  /**/  //<-- Addd space there like this /** /
  // if (FFat.begin(true)) {
  //   if (!filemgr.AddFS(FFat, "Flash/FFat", false)) {
  //     DEBUG_PORT.println(F("Adding FFAT failed."));
  //   }
  // } else {
  //   DEBUG_PORT.println(F("FFat File System not initiated."));
  // }
  /**/

  // /**/
  // if (SD_MMC.begin("/sdcard", true)) {
  //   if (!filemgr.AddFS(SD_MMC, "SD-MMC-Card", false)) {
  //     DEBUG_PORT.println(F("Adding SD_MMC failed."));
  //   }
  // } else {
  //   DEBUG_PORT.println(F("SD_MMC File System not inited."));
  // }
  // /**/

  // /**/
  // const byte SS = 5;  // D8 chip select SDCS for my 
  // if (SD.begin(SCDS)) {
  //   if (!filemgr.AddFS(SD, "SD-Card", false)) {
  //     DEBUG_PORT.println(F("Adding SD failed."));
  //   }
  // } else {
  //   DEBUG_PORT.println(F("SD File System not inited."));
  // }
  // /**/
}

uint32_t checkFileFlags(fs::FS &fs, String filename, uint32_t flags) {
  // Show file/path in Lists 
  // filenames start without "/", pathnames start with "/"
  if (flags & (ESPFMfGK::flagCheckIsFilename | ESPFMfGK::flagCheckIsPathname)) {
    /** /
    DEBUG_PORT.print("flagCheckIsFilename || flagCheckIsPathname check: ");
    DEBUG_PORT.println(filename);
    /**/
    if (flags | ESPFMfGK::flagCheckIsFilename) {
      if (filename.startsWith(".")) {
        // DEBUG_PORT.println(filename + " flagIsNotVisible");
        return ESPFMfGK::flagIsNotVisible;
      }
    }
    /*
       this will catch a pathname like /.test, but *not* /foo/.test
       so you might use .indexOf()
    */
    if (flags | ESPFMfGK::flagCheckIsPathname) {
      if (filename.startsWith("/.")) {
        // DEBUG_PORT.println(filename + " flagIsNotVisible");
        return ESPFMfGK::flagIsNotVisible;
      }
    }
  }
  
  // Checks if target file name is valid for action. This will simply allow everything by returning the queried flag
  if (flags & ESPFMfGK::flagIsValidAction) {
    return flags & (~ESPFMfGK::flagIsValidAction);
  }

  // Checks if target file name is valid for action.
  if (flags & ESPFMfGK::flagIsValidTargetFilename) {
    return flags & (~ESPFMfGK::flagIsValidTargetFilename);
  }

  // Default actions
  uint32_t defaultflags = ESPFMfGK::flagCanDelete | ESPFMfGK::flagCanRename | ESPFMfGK::flagCanGZip |  // ^t
                          ESPFMfGK::flagCanDownload | ESPFMfGK::flagCanUpload | ESPFMfGK::flagCanEdit | // ^t
                          ESPFMfGK::flagAllowPreview;

  return defaultflags;
}

void setupFilemanager(void) {
  // See above.
  filemgr.checkFileFlags = checkFileFlags;

  filemgr.WebPageTitle = "FileManager";
  filemgr.BackgroundColor = "white";
  filemgr.textareaCharset = "accept-charset=\"utf-8\"";

  // If you want authentication
  // filemgr.HttpUsername = "my";
  // filemgr.HttpPassword = "secret";

  // display the file date? change here. does not work well if you never set configTzTime()
  // filemgr.FileDateDisplay = ESPFMfGK::fddNone;

 // NOTE WIFI BEGIN IS ELSEWHERE! just start filemgr  
// if ((WiFi.status() == WL_CONNECTED) && (filemgr.begin())) {
   if (filemgr.begin()) {
     DEBUG_PORT.print(F("Open Filemanager with http://(ip)"));
    //DEBUG_PORT.print(WiFi.localIP());
    DEBUG_PORT.print(F(":"));
    DEBUG_PORT.print(filemanagerport);
    DEBUG_PORT.print(F("/"));
    DEBUG_PORT.println();
    // WifiGFXinterrupt(9, WifiStatus, "To run Filemanager\n http://                                                                   %i.%i.%i.%i:%i\n", WiFi.SSID(),
    //                    WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3],filemanagerport);
  } else {
    DEBUG_PORT.print(F("Filemanager: did not start"));
  }
}

//