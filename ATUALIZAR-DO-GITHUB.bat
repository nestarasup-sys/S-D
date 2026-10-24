@echo off
setlocal
cd /d "%~dp0"

git rev-parse --is-inside-work-tree >nul 2>&1
if errorlevel 1 goto not_repo

git remote get-url origin >nul 2>&1
if errorlevel 1 goto no_origin

git status --porcelain | findstr . >nul
if not errorlevel 1 goto dirty_worktree

git ls-remote --exit-code --heads origin main >nul 2>&1
if errorlevel 1 goto no_main

echo Buscando atualizacoes de origin/main...
git fetch origin +refs/heads/main:refs/remotes/origin/main
if errorlevel 1 goto fetch_failed

git show-ref --verify --quiet refs/heads/main
if errorlevel 1 (
    git switch --track -c main origin/main
) else (
    git switch main
)
if errorlevel 1 goto switch_failed

git pull --ff-only origin main
if errorlevel 1 goto pull_failed

echo.
echo Studio Dark atualizado com origin/main.
goto finish

:not_repo
echo ERRO: esta pasta nao e um repositorio Git.
goto failed

:no_origin
echo ERRO: nao existe um remoto chamado origin nesta copia.
goto failed

:dirty_worktree
echo ERRO: ha alteracoes locais pendentes.
echo Salve ou descarte as alteracoes antes de atualizar para evitar perda de trabalho.
goto failed

:no_main
echo ERRO: a branch main ainda nao existe no GitHub em origin.
echo O remoto atual pode usar outra branch; confira no GitHub antes de continuar.
goto failed

:fetch_failed
echo ERRO: nao foi possivel buscar origin/main. Confira a conexao e o acesso ao GitHub.
goto failed

:switch_failed
echo ERRO: nao foi possivel mudar para a branch local main.
goto failed

:pull_failed
echo ERRO: a atualizacao nao pode ser aplicada como fast-forward.
echo Confira os commits locais e remotos antes de resolver a divergencia.
goto failed

:failed
echo.
pause
exit /b 1

:finish
echo.
pause
exit /b 0
