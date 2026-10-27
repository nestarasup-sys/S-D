# Importar e revisar o resultado

1. Guarde o JSON original produzido pela outra IA; não o altere para esconder avisos.
2. No Studio Dark, escolha **Importar receita IA** e selecione o `.production.json`.
3. Gere a narração com uma voz pt-BR instalada. O aplicativo mede o áudio para resolver sincronização.
4. Revise shots, legendas e momentos de câmera na prévia. Se houver aviso, abra o frame/elemento indicado e decida se é defeito ou intenção artística.
5. Exporte um MP4 curto de validação e compare-o com a prévia.

Para analisar um projeto já compilado sem exportá-lo, no PowerShell com a pasta do app preparada, execute:

```powershell
studio_render.exe --project "projeto.studio.json" --quality-report "visual-quality.json"
```

No editor, **Qualidade visual** e **Exportar MP4** abrem a mesma revisão. O relatório salvo automaticamente fica em `Logs/visual-quality-last.json` na pasta local de dados do Studio Dark. Ele inclui `sampledFrames`; verifique esses quadros no render worker se precisar de PNGs.

Exemplos locais de referência: [receita integrada sem avisos/erros esperados](../../examples/visual-quality-review-demo-v3.production.json) e [fixture que deve gerar avisos de overflow](../../examples/visual-quality-problematic-v3.production.json). A receita integrada inclui uma citação reduzida de 68 para 32 px; `text-adapted` é informativo e não conta como aviso ou erro. O segundo arquivo é deliberadamente inválido visualmente, embora seja uma receita válida para importação.

Para compartilhar o caso com o responsável, envie o JSON original, o relatório JSON, o log de importação e os frames do relatório que mostram o problema. Não envie mídia privada sem conferir o conteúdo.
