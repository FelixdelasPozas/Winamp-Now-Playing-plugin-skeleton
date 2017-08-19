/*
    File: plugin.cpp
    Created on: 14/08/2017
    Author: Felix de las Pozas Alvarez

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
// Project
#include "plugin.h"

// C++
#include <windows.h>
#include <winuser.h>
#include <tchar.h>
#include <cstring>
#include <vector>
#include <iostream>
#include <fstream>

// Winamp SDK
#include <Winamp/wa_ipc.h>

#ifndef UNICODE
#define UNICODE
#endif
  
char PLUGIN_NAME[] = "Now Playing! v1.0";
const unsigned char note[3]{0xE2, 0x99, 0xAA};

static struct Data s_data;

/** \brief Sets our own method as the pointer to the given handle.
 * \param[in] hwnd Handle receiving messages.
 * \param[in] WndProc procesure processing those messages.
 *
 */
static WNDPROC subclass(HWND hwnd, WNDPROC WndProc)
{
  if(IsWindowUnicode(hwnd))
  {
    return (WNDPROC)SetWindowLongPtrW(hwnd,GWLP_WNDPROC,(LPARAM)WndProc);
  }

  return (WNDPROC)SetWindowLongPtrA(hwnd,GWLP_WNDPROC,(LPARAM)WndProc);
}

// Callback functions/events which will be called by Winamp
int init(void);    // called during Winamp initialization.
void config(void); // plugin configuration dialog.
void quit(void);   // called during Winamp shutdown.

// this structure contains plugin information, version, name... defined in Winamp SDK Gen.h for generic plugins.
// GPPHDR_VER is the version of the winampGeneralPurposePlugin (GPP) structure
winampGeneralPurposePlugin plugin =
{
  GPPHDR_VER,  // version of the plugin, defined in GEN.H of Winamp SDK
  PLUGIN_NAME, // name/title of the plugin
  init,        // function name which will be executed on init event
  config,      // function name which will be executed on config event
  quit,        // function name which will be executed on quit event
  0,           // handle to Winamp main window, loaded by winamp when this dll is loaded
  0            // hinstance to this dll, loaded by winamp when this dll is loaded
};

// Splits the given string at the beginning of the given delimiter string.
std::vector<std::string> split(const std::string &str, const std::string &delim)
{
  std::vector<std::string> vec;
  auto i = 0;
  auto pos = str.find(delim);
  while (pos != std::string::npos)
  {
    vec.push_back(str.substr(i, pos-i));
    i = ++pos;
    pos = str.find(delim, pos);

    if (pos == std::string::npos) vec.push_back(str.substr(i, str.length()));
  }

  return vec;
};

/** \brief Processes the messages for the given handle.
 * \param[in] hwnd Handle receiving messages.
 * \param[in] message message identifier.
 * \param[in] wParam message parameter.
 * \param[in] lParam message parameter.
 *
 */
static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  static std::string previousfile; // simple hack to only write the file once per song.

  if(lParam == IPC_CB_MISC && wParam == IPC_CB_MISC_TITLE)
  {
    char* file = (char*) SendMessage(plugin.hwndParent, WM_WA_IPC, SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETLISTPOS), IPC_GETPLAYLISTFILE);

    // only output if a valid file was found
    if (file)
    {
      // NOTE: this whole processing stuff is relative to my hard disk drive organization and different location of music.
      // You should change this to the processing you want to do. In my case I just want to dump some information that will
      // later be parsed by a OBS plugin (https://obsproject.com/) and be shown on my screencast.
      //
      std::string fileStr{file};
      if (previousfile != fileStr)
      {
        previousfile = fileStr;
        const std::string delimiter{"\\"};
        const std::string extension{".mp3"};
        std::string album, song, artist;

        auto parts = split(fileStr, delimiter);

        if (parts.size() >= 3)
        {
          song = parts.back().substr(0, parts.back().find(extension));
          auto fullAlbum = parts.at(parts.size() - 2);
          auto albumParts = split(fullAlbum, " - ");
          album = albumParts.back().substr(2, albumParts.back().length() - 1);
          artist = albumParts.front();
        }

        std::ofstream txtFile;
        txtFile.open(s_data.pluginPath + "\\NowPlaying.txt", std::ios::out | std::ios::binary);
        if (txtFile.is_open())
        {
          txtFile.write((char*) note, 3);
          txtFile << u8" Listening to '" << album << u8"' by " << artist << u8". Song: '" << song << u8"'. ";
          txtFile.close();
        }
      }
    }
  }

  return CallWindowProc(s_data.lpWndProcOld,hwnd,message,wParam,lParam);
}


//--------------------------------------------------------------------
int init()
{
  // Fill in the Data struct.
  s_data.pluginPath = (char *)SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETPLUGINDIRECTORY);

  // get the main window handle
  auto hWndPE = (HWND)SendMessage(plugin.hwndParent,WM_WA_IPC,IPC_GETWND_PE, IPC_GETWND);

  if (!hWndPE) return 0;

  // be a proxy for the main window to receive messages before it does.
  s_data.lpWndProcOld = subclass(plugin.hwndParent,WndProc);

  return 0;
}

//--------------------------------------------------------------------
void config()
{
  // Plugin config button, just show a dialog.
  MessageBoxW(plugin.hwndParent, L"©2017 Felix de las Pozas Alvarez.\n", L"Now Playing! Plugin v1.0", MB_OK);
}

//--------------------------------------------------------------------
void quit()
{
  // nothing to be done at shutdown.
}

// We wrap the code in 'extern "C"' to ensure the export isn't mangled if used in a CPP file.
extern "C" __declspec(dllexport) winampGeneralPurposePlugin * winampGetGeneralPurposePlugin()
{
  return &plugin;
}

