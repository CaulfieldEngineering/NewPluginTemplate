# Product regression scripts

This plugin's own regression checks, one per bug found. `plugin/tests/run_all.ps1` runs every `.py` in this folder as:

```
python <script>.py <ReviewRender executable>
```

Exit code 0 = pass. See `../examples/` for Tertiary's, which show the pattern.
