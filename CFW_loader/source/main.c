#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "fs.h"
#include "hid.h"
#include "textmenu.h"
#include "platform.h"
#include "draw.h"

//This contains the System Firmware Version String.
char* cfw_FWString;
//This is the System Firmware Version in system.txt
char cfw_FWValue;
//if true, dump the ARM9 ram
bool cfw_arm9dump;

//[Unused]
//void ioDelay(u32);

// @breif  [Unused]Simply wait for a key, but not use its value.
// @note   Work like a pause.
void CFW_WaitKey(void) {
	DrawDebug(1,"");
	DrawDebug(1,"Press key to continue");
	HidWaitForInput();
}

// @breif  This reads the system version from the configuration file.
// @note   So please make sure you've have the file in your SD.
//         Also, if we use binary data to store the system version..
void CFW_getSystemVersion(void) {
	char settings[16];
	if (FSFileOpen("/3ds/PastaCFW/system.txt")){
		FSFileRead(settings, 16, 0);
		FSFileClose();
	}
    cfw_FWValue = settings[0];
	switch (settings[0]) {
    	case '1': // 4.x
    		cfw_FWString = platform_FWStrings[0];
    		break;
    	case '2': // 5.0
    		cfw_FWString = platform_FWStrings[1];
    		break;
    	case '3': // 5.1
    		cfw_FWString = platform_FWStrings[2];
    		break;
    	case '4': // 6.0
    		cfw_FWString = platform_FWStrings[3];
    		break;
    	case '5': // 6.1 - 6.3
    		cfw_FWString = platform_FWStrings[4];
    		break;
    	case '6': // 7.0-7.1
    		cfw_FWString = platform_FWStrings[5];
    		break;
    	case '7': // 7.2
    		cfw_FWString = platform_FWStrings[6];
    		break;
    	case '8': // 8.x
    		cfw_FWString = platform_FWStrings[7];
    		break;
    	case '9': // 9.x
    		cfw_FWString = platform_FWStrings[8];
    		break;
    	case 'a': // 8.x
    		cfw_FWString = platform_FWStrings[9];
    		break;
    	case 'b': // 9.x
    		cfw_FWString = platform_FWStrings[10];
    		break;
	}
	//Check if to use the ARM9 Dumper
	if (settings[2] == '1') cfw_arm9dump = true;
}

// @breif  Patch the offsets to pass the signs.
void CFW_SecondStage(void) {
    u8 patchO0[] = { 0x00, 0x20, 0x3B, 0xE0 };
	u8 patchO1[] = { 0x00, 0x20, 0x08, 0xE0 };
	u8 patchN0[] = { 0x01, 0x27, 0x1E, 0xF5 };
	u8 patchN1[] = { 0xB4, 0xF9, 0xD0, 0xAB };
	u8 patchN2[] = { 0x6D, 0x20, 0xCE, 0x77 };
	u8 patchN3[] = { 0x5A, 0xC5, 0x73, 0xC1 };
	//Apply patches
	DrawDebug(0,"Apply patch for type %c...", cfw_FWValue);
    switch(cfw_FWValue) {
        //Old-3DS
    	case '1': // 4.x
    		memcpy((u32*)0x080549C4, patchO0, 4);
    		memcpy((u32*)0x0804239C, patchO1, 4);
    		break;
    	case '2': // 5.0
    		memcpy((u32*)0x08051650, patchO0, 4);
    		memcpy((u32*)0x0803C838, patchO1, 4);
    		break;
    	case '3': // 5.1
    		memcpy((u32*)0x0805164C, patchO0, 4);
    		memcpy((u32*)0x0803C838, patchO1, 4);
    		break;
    	case '4': // 6.0
    		memcpy((u32*)0x0805235C, patchO0, 4);
    		memcpy((u32*)0x08057FE4, patchO1, 4);
    		break;
    	case '5': // 6.1 - 6.3
    		memcpy((u32*)0x08051B5C, patchO0, 4);
    		memcpy((u32*)0x08057FE4, patchO1, 4);
    		break;
    	case '6': // 7.0-7.1
    		memcpy((u32*)0x080521C4, patchO0, 4);
    		memcpy((u32*)0x08057E98, patchO1, 4);
    		break;
    	case '7': // 7.2
    		memcpy((u32*)0x080521C8, patchO0, 4);
    		memcpy((u32*)0x08057E9C, patchO1, 4);
    		break;
    	case '8': // 8.x
    		memcpy((u32*)0x080523C4, patchO0, 4);
    		memcpy((u32*)0x08058098, patchO1, 4);
    		break;
    	case '9': // 9.x
    		memcpy((u32*)0x0805235C, patchO0, 4);
    		memcpy((u32*)0x08058100, patchO1, 4);
    		break;
        //New-3DS
    	case 'a': // 8.x
    		memcpy((u32*)0x08053114, patchN0, 4);
    		memcpy((u32*)0x080587E0, patchN1, 4);
    		break;
    	case 'b': // 9.x
    		memcpy((u32*)0x08052FD8, patchN2, 4);
    		memcpy((u32*)0x08058804, patchN3, 4);
    		break;
    }
	DrawDebug(1,"Apply patch for type %c...                  Done!", cfw_FWValue);
}

// @breif  Load patches from file and patch them if enabled after compared or overrided.
// @note   Patch source: _patch.bin. Behaviour only when enabled in system.txt.
//         >>Read the patches one after one.
//           >>Enabled?
//             >>Loading patched code and overrided code
//             >>Y Overrided?>>Y Just patch it, yep.
//                           >>N patch the memory when current matches overrided.
//         This feature may easily make troubles so please take care and use tools.
void CFW_CustomPatch(void) {
    u8 fileVersion = 0, patchesCount = 0, patchFlag = 0, patchSize = 0;
    u32 patchAddr = 0;
    u8 i = 0, j = 0, failedPatches = 0; // i for patches, j for checking
    u8 patchbuf[256] = {0,};
    UINT expectedBytes = 0;
    //This file structure: (version 0)
    //Head: u8 version; u8 count;
    //Body contains records count as patchesCount.
    //u32 addr; u8 flag; u8 size; u8 expected[]; u8 patch[];
    //where flag: Bit0 - enabled? Bit1 - just override?
    if (f_open(&fsFile, "/3ds/PastaCFW/_patch.bin", FA_READ | FA_OPEN_EXISTING) != FR_OK) {
        f_close(&fsFile);
        DrawDebug(1,"CP.ERR:_patch.bin not found");
        return;
    }
    f_lseek(&fsFile, 0);
    f_read(&fsFile, patchbuf, 2, &expectedBytes);
    if (expectedBytes!=2) {
        f_close(&fsFile);
        DrawDebug(1,"CP.ERR:binary corrupted");
        return;
    }
    fileVersion = patchbuf[0]; patchesCount = patchbuf[1];
    if (fileVersion != 0) {
        f_close(&fsFile);
        DrawDebug(1,"CP.ERR:version %u unsupported", fileVersion);
        return;
    }
    for (i = 0; i < patchesCount; i ++) {
        f_read(&fsFile, &patchAddr, 4, &expectedBytes);
        if (expectedBytes!=4) {
            f_close(&fsFile);
            DrawDebug(1,"CP.ERR:binary corrupted");
            return;
        }
        if (!patchAddr) {
            f_close(&fsFile);
            DrawDebug(1,"CP.ERR:invalid address 0x%X", patchAddr);
            return;
        }
        f_read(&fsFile, &patchFlag, 1, &expectedBytes);
        if (expectedBytes!=1) {
            f_close(&fsFile);
            DrawDebug(1,"CP.ERR:binary corrupted");
            return;
        }
        f_read(&fsFile, &patchSize, 1, &expectedBytes);
        if (expectedBytes!=1) {
            f_close(&fsFile);
            DrawDebug(1,"CP.ERR:binary corrupted");
            return;
        }
        if (!patchSize) {
            f_close(&fsFile);
            DrawDebug(1,"CP.ERR:invalid size 0x%X", patchSize);
            return;
        }
        f_read(&fsFile, &patchbuf, patchSize, &expectedBytes);
        if (expectedBytes!=patchSize) {
            f_close(&fsFile);
            DrawDebug(1,"CP.ERR:binary corrupted");
            return;
        }
        if (!(patchFlag & 0x2)) {
            // doesn't override, so start testing.
            for (j = 0; j < patchSize; j++) {
                if (*(u8*)(patchAddr + j) != patchbuf[j]) {
                    DrawDebug(1,"CP.INF:chunk mismatch %u", i);
                    patchFlag = 0; failedPatches++;
                    break;
                }
            }
        }
        f_read(&fsFile, &patchbuf, patchSize, &expectedBytes);
        if (expectedBytes!=patchSize) {
            f_close(&fsFile);
            DrawDebug(1,"CP.ERR:binary corrupted");
            return;
        }
        if (patchFlag & 0x1) {
            // If enabled and matches or overrided.
            memcpy((u32*)patchAddr, patchbuf, patchSize);
        }
    }
    f_close(&fsFile);
    DrawDebug(1,"CP.INF:done, failed %u", failedPatches);
}

// @breif  Dump ARM9 Ram to file.
void CFW_ARM9Dumper(void) {
	DrawDebug(1,"");
	DrawDebug(1,"");
	DrawDebug(1,"");
	DrawDebug(1,"---------------- ARM9 RAM DUMPER ---------------");
	DrawDebug(1,"");
	DrawDebug(0,"Press A to DUMP, B to skip.");

	u32 pad_state = HidWaitForInput();
	if (pad_state & BUTTON_B) DrawDebug(1,"Skipping...");
	else {
		UINT bytesWritten = 0, currentWritten = 0;
		//u32 result = 0;
		u32 currentSize = 0;
		void *dumpAddr = (void*)0x08000000;
		u32 fullSize = 0x00100000;
		const u32 chunkSize = 0x10000;

		if (FSFileCreate("/3ds/PastaCFW/RAM.bin", true)) {
            f_lseek(&fsFile, 0);
			while (currentWritten < fullSize) {
				currentSize = fullSize - currentWritten < chunkSize ? fullSize - currentWritten : chunkSize;
                f_write(&fsFile, (u8 *)dumpAddr + currentWritten, currentSize, &bytesWritten);f_sync(&fsFile);
				if (bytesWritten != currentSize) break;
				currentWritten += bytesWritten;
				DrawDebug(0,"Dumping:                         %07d/%d", currentWritten, fullSize);
			}
			FSFileClose();
			//result = (fullSize == currentWritten);
		}
		DrawDebug(1,"");
		DrawDebug(1,"");
		DrawDebug(1,"Dump %s! Press any key to boot CFW.", (fullSize == currentWritten) ? "finished" : "failed");
		HidWaitForInput();
	}
}

// @breif  Main routine surely.
int main(void) {
	//BOOT
	DrawClearScreenAll();
	DrawDebug(1,"--------------- PASTA CFW LOADER ---------------");
	DrawDebug(1,"");
	DrawDebug(0,"Initializing FS...");
	FSInit();
	DrawDebug(1,"Initializing FS...                         Done!");
	DrawDebug(1,"");
	DrawDebug(0,"Getting system information...");
	CFW_getSystemVersion();
	DrawDebug(1,"Getting system information...              Done!");
	DrawDebug(1,"");
	DrawDebug(1,"Your system is %s", cfw_FWString);
	DrawDebug(1,"");
	CFW_SecondStage();
	if (cfw_arm9dump == true) CFW_ARM9Dumper();

	// return control to FIRM ARM9 code (performs firmlaunch)
	return 0;
}
