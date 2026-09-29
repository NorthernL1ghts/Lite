# To-do

1. **Delete and duplicate entities.** Done. Right-click an entity, or use the Duplicate and Delete buttons. Ctrl+D copies the selection. Del removes it, and holding Del keeps removing the next entity.
2. **Remove a component.** Done. Each component in the inspector has a Remove button. Physics updates immediately, including while playing.
3. **Viewport gizmos.** Done. Drag a selected entity to move it. Corner squares scale it, and the ring rotates it. The inspector fields still type and slide.
4. **Drag from the explorer.** Done. Drag a texture onto a material to assign it. The first image dropped into an empty Background plane fills the view. Later drops are editable sprites in front of it. Opacity blends them. Drag a scene file onto the viewport or the scene panel to open it.
5. **Prefabs.** The Prefabs folder is empty. Save an entity as a prefab, then place copies of it into a scene.
6. **Scripts.** The Scripts folder and `ScriptModulePath` are stored on the project. Nothing loads or runs a script yet.
7. **Sandbox uses the project.** The player still opens `assets/scenes/Sandbox.scene` directly. It should open `Sandbox.lite` and play that project's start scene.
8. **Planes affect drawing.** Planes currently only group the sidebar. Give each plane a draw order so everything on a higher plane renders in front.
