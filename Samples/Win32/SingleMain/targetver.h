#pragma once

// Durch Einbeziehen von"SDKDDKVer.h" wird die höchste verfügbare Windows-Plattform definiert.

// Wenn Sie die Anwendung für eine frühere Windows-Plattform erstellen möchten, schließen Sie "WinSDKVer.h" ein, und
// legen Sie das _WIN32_WINNT-Makro auf die zu unterstützende Plattform fest, bevor Sie "SDKDDKVer.h" einschließen.

//#include <SDKDDKVer.h>

// Die folgenden Makros definieren die mindestens erforderliche Plattform. Die mindestens erforderliche Plattform
// ist die früheste Windows-, Internet Explorer-Version usw., die über die erforderlichen Features zur Ausführung 
// Ihrer Anwendung verfügt. Die Makros aktivieren alle Funktionen, die auf den Plattformversionen bis 
// einschließlich der angegebenen Version verfügbar sind.

// Ändern Sie folgende Definitionen für Plattformen, die älter als die unten angegebenen sind.
// Unter MSDN finden Sie die neuesten Informationen über die entsprechenden Werte für die unterschiedlichen Plattformen.
#ifndef _WIN32_WINNT            // Gibt an, dass Windows Vista die mindestens erforderliche Plattform ist.
#define _WIN32_WINNT 0x0600     // Ändern Sie den entsprechenden Wert, um auf andere Versionen von Windows abzuzielen.
#endif
