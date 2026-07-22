import importlib.util
spec = importlib.util.spec_from_file_location('dt', 'c:/Users/Atendio - Comercial/Documents/GitHub/cli/tiktok-downloader/download_tiktok.py')
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)
print(mod.is_bot_blocked('Please wait while we verify you are human'))
print(mod.is_bot_blocked('<html><body>normal content</body></html>'))
