# Gradient Data graphical interface

Gradient Data 0.3.0-rc.4 includes an offline graphical interface for Linux and Windows x86-64. It uses your installed browser and the same native engine as the CLI. No Node, Python or Rust is required to run the portable interface. Close it with **Close session** to stop its local background process; merely closing the tab leaves the process available for reopening.

On Windows, double-click `Start_Gradient_Data.cmd` or `windows-x86_64/uacd-gui.exe`. On Linux, run `./start-gui.sh` or open the executable `linux-x86_64/uacd-gui` with your file manager. If an extractor removes Linux executable permissions, mark the launcher and executable as runnable. Optional installation also supplies the `uacd-gui` user-local command. If automatic opening fails, `uacd gui` prints a private local URL to open manually.

1. Choose **Create archive**, then **Browse files & folders**. Browse to a source folder, or select multiple files/folders. The directory picker has an Up button and starting places; selected roots appear in the selection list.
2. Choose an output outside your selected source folders, with a new `.uacd` filename. Existing files are never replaced.
3. Keep the default quality / 256 KiB / entropy off / Auto, or choose validated settings. **CPU workers** offers Auto, Max, and Custom 1–64; the result reports the selected limit and any CPU/memory admission reason. TALON is very slow and supports at most 256 KiB; profile and block size are separate settings.
4. Start and watch progress. Cancellation is checked between blocks/files, including before publication; a long TALON block must finish before cancellation takes effect.
5. Choose **Open & restore**, then inspect, verify or restore into a new folder. Folder roots retain their names, e.g. restoring `Game` produces `OUTPUT/Game/...`.
6. Select one listed file to extract it separately or read a byte range. The full folder does not need to be restored.

The interface listens on `127.0.0.1` only, uses a fresh private session secret and does not upload data. Do not share the startup URL. Filesystem actions require the exact local origin and session secret. This is a local desktop convenience, not a remotely hosted service or 7-Zip plugin.

GUI read admission: 1 TiB declared source/archive bytes, 64 MiB aggregate index and 100000 entries. Folder archives have fixed safety ceilings of 64 MiB index and 100000 entries. The picker shows 200 rows per page and accepts directories with up to 10000 immediate entries. It supports UTF-8 paths; unsupported links, special files or nonportable names fail explicitly. Only one operation runs at a time. The archive does not preserve ACLs, ownership, timestamps or hard-link identity; it preserves bytes, paths, empty folders and Unix executable flags.

Destination parents must be trusted and must remain unchanged. Integrity hashes detect damaged bytes, not an untrusted author or encrypted contents. This unsigned release is for evaluation and integration testing under `NOTICE.txt`; it is not a production-security certification.

The native GUI attaches an at-most 2 GiB process quota once at session startup, before jobs. Linux accounts address space; Windows uses conservative committed-memory accounting. This is not an RSS cap or guaranteed reservation. Auto leaves one detected logical CPU outside its worker limit when more than two are available; Max targets all detected CPUs, capped at 64 and by memory estimates. Custom N must fit the detected CPU/memory admission. Unavailable memory/quota snapshots make Auto choose one worker. Short members may start fewer threads. The automatically launched browser starts before the GUI attaches its new quota; external system limits may still apply to both processes. Use the CLI for a different memory-budget setting.
