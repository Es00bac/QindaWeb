# QindaWeb

A native QindaTK browser for the QindaQt desktop. Each browsing view owns one Qt window; the desktop provides tabs and splits across those windows.

The production target is Mozilla Gecko behind a native Qt6 content adapter. Browser chrome, profiles and session state belong to QindaWeb; authenticated placement belongs to QindaQt. Development and engine qualification are tracked separately so an available experimental backend is never mistaken for a finished Gecko runtime.
