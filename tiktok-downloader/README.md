# TikTok Downloader

CLI simples em C para baixar vídeos de uma página do TikTok usando curl.

## Estrutura

- src/main.c: interface CLI e fluxo principal
- src/downloader.c: download dos vídeos
- src/parser.c: extração de URLs de vídeo a partir do HTML
- src/curl_utils.c: obtenção da página alvo
- downloads/: pasta para os arquivos baixados

## Compilar

```bash
make
```

## Executar

```bash
./tiktok-dl https://www.tiktok.com/@usuario 10
```

Ou:

```bash
./tiktok-dl --limit 10 https://www.tiktok.com/@usuario
```

## Observações

- O TikTok pode bloquear ou exigir conteúdo dinâmico, então nem todas as páginas expõem URLs diretamente no HTML.
- O programa salva os arquivos em downloads/ com nomes como video_01.mp4.
- O painel visual foi inspirado no estilo de um CLI bonito com banner e progresso.
