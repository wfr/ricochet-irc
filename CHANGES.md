2022-04-28:
 * Headless build:
   `cmake -D ENABLE_GUI=OFF (default)`

2022-04-24:
 * Merged ricochet-refresh v3.0.11:
   - Upgrade tor dependencies to version 0.4.6.10
   - Upgrade openssl to version 1.1.1n
   - Fixed #124: Silently migrate profile from
     QStandardPaths::AppLocalDataLocation to QStandardPaths::AppConfigLocation
     (manually selected profile path is not affected)
   - Migrated build system to CMake
   * NOTE: Config dir moved from `~/.local/share/ricochet-refresh` to
     `~/,config/ricochet-refresh`.
 * IRC:
   - Migrated build system to CMake
   - Fixed new profile creation.

2020-11-22:
 * Rebased on Ricochet Refresh, v3-2020-alpha branch
 * Restored compatibility with GCC 8
 * Added v3 onion support. Removed v2 onion support
 * TODO: clean up, remove GUI dependencies

2017-01-15:
 * small UX improvements (input sanitation and validation)
 * prevent nickname conflicts

2017-01-13:
 * updated to v1.1.4

