# Checklist de release

## Pré-build

- [ ] `src/` sem `TODO` em caminho principal não documentado.
- [ ] `CMakeLists.txt` com versão correta.
- [ ] Dependências novas com ADR e licença registradas.
- [ ] `docs/audit/` do escopo aprovado.

## Build

- [ ] Build limpo do zero (apagar `build/`).
- [ ] Sem aviso de toolchain.
- [ ] `ctest --preset unit-tests` passando; saída anexada.
- [ ] `ArtPlus.exe` abre e fecha.

## Pacote

```powershell
$dist = 'H:\Desktop\ArtPlus\dist\ArtPlus'
Remove-Item -Recurse -Force $dist -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $dist | Out-Null
Copy-Item H:\Desktop\ArtPlus\build\ArtPlus.exe $dist
& 'C:\Qt\6.8.3\mingw_64\bin\windeployqt.exe' --release --compiler-runtime --no-translations --dir $dist "$dist\ArtPlus.exe"
Compress-Archive -Path "$dist\*" -DestinationPath H:\Desktop\ArtPlus\dist\ArtPlus-0.1.0-win64.zip -Force
```

- [ ] DLLs Qt copiadas (Core, Gui, Widgets, Qml).
- [ ] Plugins `platforms/qwindows.dll` presentes.
- [ ] `libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll` presentes.
- [ ] Zip com versão no nome.
- [ ] Testar o zip em pasta limpa, sem Qt no PATH.

## Publicação

- [ ] `git status` sem `build/` ou `dist/`.
- [ ] Tag `vX.Y.Z` criada.
- [ ] `gh release create` com notas.
- [ ] Artefato anexado.

## Pós

- [ ] Smoke em máquina sem Qt instalado.
- [ ] Registrar problema conhecido no `ROADMAP.md`.
