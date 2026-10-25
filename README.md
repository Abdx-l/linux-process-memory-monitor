# Linux Process and Memory Monitor

Linux /proc process monitor with robust stat-name parsing, CPU delta and resident-memory accounting, ncurses sorting/selection, confirmed SIGTERM, sustained-threshold daemon alerts, and systemd packaging.

## Run

Install ncurses development headers. `make`, then `./procmon`; c/m selects CPU/memory sorting, arrows select, k then y sends SIGTERM, q quits. `./procmon --daemon [log-path]` records processes above 80% CPU or 512 MiB for 10 samples. Daemon runs under a systemd service account with only log-directory write access.
