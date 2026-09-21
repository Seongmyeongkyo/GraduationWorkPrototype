"""Run once with UnrealEditor-Cmd -run=pythonscript -script=<this file>.

Requires PythonScriptPlugin for this editor run only.
Existing maps are never overwritten. Runtime menu code does not require Python.
"""
import unreal

MAP_PATH = "/Game/Map/MainMenu"
if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
    raise RuntimeError("MainMenu already exists; inspect it in the editor instead of recreating it.")

menu_mode = unreal.load_class(None, "/Script/Framework.FWMainMenuGameMode")
if menu_mode is None:
    raise RuntimeError("Build FrameworkEditor before creating the menu map.")
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not levels.new_level(MAP_PATH):
    raise RuntimeError("Could not create MainMenu map.")
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", menu_mode)
if not levels.save_current_level():
    raise RuntimeError("Could not save MainMenu map.")
unreal.log("FW_MENU_MAP_CREATED: " + MAP_PATH)
