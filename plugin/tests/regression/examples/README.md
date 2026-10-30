# Examples (from Tertiary, not run)

These two scripts are Tertiary's, copied unchanged as examples of a product regression check. They use Tertiary's parameter names ("Rate Low Band", "Sync to Host Low Band", ...), so they don't run against this plugin, and `run_all.ps1` doesn't run this folder.

- `lfo_rate_step.py`: written after Joe heard a free-running band "go haywire" when its rate changed. Renders a DC input through ReviewRender with the rate held, and with the rate stepped mid-run, and checks the stepped render has no jump bigger than the steady one.
- `lfo_host_lock.py`: checks synced LFOs land on the host's beat grid after a loop, a locate, and a stop and start (ReviewRender's `--jump-at` and `--stop`).

The pattern: one script per bug, takes the ReviewRender path, exit code 0 = pass. Put this plugin's own in `../product/`.
