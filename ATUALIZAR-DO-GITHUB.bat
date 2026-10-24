@echo off
setlocal
cd /d "%~dp0"

git rev-parse --is-inside-work-tree >nul 2>&1
if errorlevel 1 goto not_repo

git remote get-url origin >nul 2>&1
if errorlevel 1 goto no_origin

git status --porcelain | findstr . >nul
if not errorlevel 1 goto dirty_worktree

git ls-remote --exit-code --heads origin master >nul 2>&1
if errorlevel 1 goto no_master

echo Buscando atualizacoes de origin/master...
git fetch origin +refs/heads/master:refs/remotes/origin/master
if errorlevel 1 goto fetch_failed

git show-ref --verify --quiet refs/heads/master
if errorlevel 1 (
    git switch --track -c master origin/master
) else (
    git switch master
)
if errorlevel 1 goto switch_failed

git pull --ff-only origin master
if errorlevel 1 goto pull_failed

echo.
echo Studio Dark atualizado com origin/master.
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

:no_master
echo ERRO: a branch master nao existe no GitHub em origin.
echo Confira o nome da branch no GitHub antes de continuar.
goto failed

:fetch_failed
echo ERRO: nao foi possivel buscar origin/master. Confira a conexao e o acesso ao GitHub.
goto failed

:switch_failed
echo ERRO: nao foi possivel mudar para a branch local master.
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
