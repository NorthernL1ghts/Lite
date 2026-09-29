# To-do

1. **Delete and duplicate entities.** Done. Right-click an entity, or use the Duplicate and Delete buttons. Ctrl+D copies the selection. Del removes it, and holding Del keeps removing the next entity.
2. **Remove a component.** The inspector can add a component. It cannot take one off.
3. **Viewport gizmos.** Drag a selected entity in the view to move it, with rotate and scale handles. The inspector fields already type and slide.
4. **Drag from the explorer.** Drop a texture onto a material, and drop a scene file to open it.
5. **Prefabs.** The Prefabs folder is empty. Save an entity as a prefab, then place copies of it into a scene.
6. **Scripts.** The Scripts folder and `ScriptModulePath` are stored on the project. Nothing loads or runs a script yet.
7. **Sandbox uses the project.** The player still opens `assets/scenes/Sandbox.scene` directly. It should open `Sandbox.lite` and play that project's start scene.
8. **Planes affect drawing.** Planes currently only group the sidebar. Give each plane a draw order so everything on a higher plane renders in front.
