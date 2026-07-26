import time

Salamander.ui.notify("CPython extension package is running through Salamatrix.", "Salamatrix Python Demo", 2500)
progress = Salamander.ui.progress("Salamatrix Python Progress Demo", 5)
try:
    for step in range(1, 6):
        progress.update(step, text=f"Step {step} of 5")
        time.sleep(0.15)
        if progress.is_cancelled():
            break
finally:
    progress.close()
Salamander.storage.set("lastRun", "Python.CPython")
