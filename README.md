# uffi
Lua wrapper for GetProcAddress

## Usage
```
loadlib = require "uffi"
msgbox = loadlib("int MessageBoxA (int, char *, char *, int)", "user32.dll")
print(msgbox(0, "Text", "Caption", 1))
```
