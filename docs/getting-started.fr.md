# Bien démarrer

Comment reprendre CatWandConnect sur une nouvelle machine, que tu sois le mainteneur ou un nouveau contributeur.

## Où en est le projet

Le firmware dédié de `firmware/` remplace l'ancienne preuve de concept ESPHome. La V1 ([issue #1](https://github.com/vistorx/CatWandConnect/issues/1)) est terminée : la v0.1.0, première version stable, a passé la [checklist de recette](acceptance-checklist.fr.md) sur un ESP32 classique. Les versions pour ESP32-S3 et ESP32-C3 sont testées automatiquement, mais pas encore sur du vrai matériel. Le travail prévu porte le label [`future`](https://github.com/vistorx/CatWandConnect/issues?q=is%3Aopen+label%3Afuture).

## À lire d'abord

| À lire | Pour |
|--------|------|
| [`CONTEXT.md`](../CONTEXT.md) | Le glossaire (en anglais). Utilise ces mots dans le code, les issues et la doc. |
| [`docs/adr/`](adr/) | Pourquoi les grandes décisions ont été prises : tout ouvrir, autonome d'abord, Arduino + PlatformIO, MQTT pour Home Assistant, licences, interface web simple (en anglais). |
| [Issue #1](https://github.com/vistorx/CatWandConnect/issues/1) | La spec V1. Ses sous-issues sont les tickets, avec des liens natifs « blocked by ». |
| [`AGENTS.md`](../AGENTS.md) et [`docs/agents/`](agents/) | Comment les agents IA travaillent avec le suivi des issues, les labels et la doc du domaine (en anglais). |

## Ce qui est dans le dépôt, et ce qui n'y est pas

Dans le dépôt :

- Logo, bannières, photos et captures d'écran dans `media/` (voir [`media/README.md`](../media/README.md)).
- La doc du domaine, les ADR et la configuration des agents.
- Les pièces à imprimer du Mk3, le mécanisme actuel, dans `stls/mk3/`.
- Les réglages de projet Claude Code qui activent le plugin `mattpocock-skills` (`.claude/settings.json`).

Pas dans le dépôt :

- **Les sources CAO.** Seuls les exports STL/3MF sont versionnés, et `cads/` est vide. Pour modifier une pièce, il faut le projet CAO d'origine.
- **La preuve de concept ESPHome.** Elle a été supprimée dans `fa58005`. Récupère-la avec `git show 3cda6f7:sources/cat-playing-platform.yaml`. C'est la référence pour la logique de mouvement de la preuve de concept.
- **Les secrets.** Aucun n'est versionné, et le nouveau firmware n'en a besoin d'aucun à la compilation. Les identifiants Wi-Fi se saisissent sur l'appareil.
- **La configuration propre à chaque machine.** Ta clé SSH, ta connexion GitHub, ton identité git et `.claude/settings.local.json` sont à recréer sur chaque machine. Voir ci-dessous.

## Préparer une machine

### 1. Outils

- Git et la [CLI GitHub](https://cli.github.com/) (`gh`).
- VS Code avec l'extension **PlatformIO IDE**. Elle fournit la chaîne de compilation pour ESP32, ESP32-S3 et ESP32-C3, plus l'environnement de tests natif.
- Facultatif : [Claude Code](https://claude.com/claude-code). Le plugin `mattpocock-skills` est activé pour ce workspace par les réglages versionnés. Accepte la proposition d'installation la première fois.

### 2. Cloner

Les contributeurs peuvent cloner normalement :

```sh
git clone git@github.com:vistorx/CatWandConnect.git
```

Ne clone pas dans un dossier synchronisé par Nextcloud, Dropbox ou iCloud. Synchroniser un dossier `.git` entre plusieurs machines le corrompt. Chaque machine a son propre clone, et git sert de synchronisation.

### 3. Mainteneur uniquement : utiliser le compte `vistorx` sur une machine qui a un autre compte GitHub

Sur une machine de travail, le compte GitHub par défaut est celui de quelqu'un d'autre. Ces étapes limitent `vistorx` à ce seul dépôt.

**Une clé SSH, avec un hôte alias pour ne pas toucher au `github.com` par défaut :**

```sh
ssh-keygen -t ed25519 -C "vistorx" -f ~/.ssh/github_vistorx
# Ajoute ~/.ssh/github_vistorx.pub sur https://github.com/settings/keys (connecté en tant que vistorx)
```

Ajoute ceci à la fin de `~/.ssh/config`. Le port 443 passe sur les réseaux qui bloquent le port 22.

```
Host github-vistorx
  HostName ssh.github.com
  Port 443
  User git
  IdentityFile ~/.ssh/github_vistorx
  IdentitiesOnly yes
  AddKeysToAgent yes
  UseKeychain yes
```

Vérifie avec `ssh -T github-vistorx`, qui doit saluer `vistorx`.

**Cloner via l'alias et définir une identité locale au dépôt :**

```sh
git clone git@github-vistorx:vistorx/CatWandConnect.git
cd CatWandConnect
git config user.name "vistorx"
git config user.email "vistorx@users.noreply.github.com"
```

**Une connexion `gh` séparée, stockée dans son propre dossier de configuration :**

```sh
GH_CONFIG_DIR=~/.config/gh-vistorx gh auth login --web
```

Puis fais pointer ce workspace vers ce dossier :

- Pour Claude Code, crée `.claude/settings.local.json`. Il est ignoré par git.

  ```json
  { "env": { "GH_CONFIG_DIR": "/Users/<you>/.config/gh-vistorx" } }
  ```

- Pour le terminal de VS Code, ajoute ceci dans les `settings` de `CatWandConnect.code-workspace`, et ne versionne pas ce changement :

  ```json
  "terminal.integrated.env.osx": { "GH_CONFIG_DIR": "${env:HOME}/.config/gh-vistorx" }
  ```

Vérifie avec `gh auth status` dans le terminal du workspace, qui doit afficher `vistorx`.

### 4. Compiler et tester

Le firmware est un seul projet PlatformIO dans `firmware/`. Lance ces commandes depuis ce dossier, ou utilise la barre latérale PlatformIO de VS Code. L'extension VS Code installe `pio` dans `~/.platformio/penv/bin/pio`.

```sh
pio test -e native     # tests du Robot core sur ton ordinateur, sans carte
pio run                # compile pour ESP32, ESP32-S3 et ESP32-C3
pio run -e esp32 -t upload -t monitor   # flashe une carte en USB et affiche ses logs
```

Pour travailler sur l'interface du robot sans robot, lance `node tools/dev_server.mjs` (Node 18+, rien à installer) et ouvre <http://localhost:8080/>. Il sert `firmware/web/` avec un robot simulé sommaire derrière la même API.

Les environnements sont `esp32`, `esp32s3`, `esp32c3` et `native`. La CI (`.github/workflows/ci.yml`) lance les tests et les trois compilations à chaque push et pull request.

Organisation du code :

- `firmware/lib/robot_core/` est le Robot core. C'est du C++17 pur qui n'inclut jamais d'en-têtes Arduino. Il n'accède au matériel que par les interfaces `MotorDriver`, `Clock` et `Storage` de `ports.h`.
- `firmware/src/` est le côté Arduino. Il implémente ces interfaces et les relie au Robot core.
- `firmware/src/network/` met le robot sur le Wi-Fi : configuration Improv, hotspot de secours avec sa page de configuration, et mDNS. `firmware/lib/improv/` est le découpage du protocole Improv, en C++ pur et testé.
- `firmware/web/` est l'interface du robot : HTML, CSS et JS simples, sans étape de build. Les textes sont dans `web/lang/*.json`. Pour ajouter une langue, copie `en.json`, traduis-le, ajoute-le à `LANGUAGES` dans `i18n.js`, et lance `node tools/check_lang.mjs` (la CI le lance aussi). `tools/embed_web.py` la compresse dans le firmware à la compilation. L'interface ne parle au robot que par l'[API du robot](api.fr.md).
- `firmware/test/` contient les tests natifs, avec des simulations en mémoire des interfaces.
- La [checklist de recette](acceptance-checklist.fr.md) (`docs/acceptance-checklist.md`) liste ce qu'il faut vérifier sur le vrai robot. Complète-la chaque fois que tu ajoutes un comportement que les tests ne peuvent pas couvrir.

Le firmware utilise la table de partitions `min_spiffs` : deux emplacements d'application de 1,9 Mo pour les mises à jour OTA, et une organisation sur 4 Mo pour toutes les puces, pour que les cartes de 8 Mo marchent aussi. Les cartes flashées avant l'adoption de cette organisation ont besoin d'un flash en USB pour en changer ; ensuite, les mises à jour peuvent passer par le Wi-Fi.

Chaque compilation produit aussi `factory.bin`, l'image complète de la flash (elle efface les réglages ; le flasher web l'écrit en deux parties qui les épargnent). La publication d'une version est décrite dans [Publier une version](releasing.md) (en anglais) ; la page du flasher est `flasher/index.html`, et le site du projet est construit à partir du README et de `docs/` (voir [The website and the web flasher](releasing.md#the-website-and-the-web-flasher)). Le site est en anglais et en français : chaque page utilisateur `page.md` a sa traduction `page.fr.md` à côté, et `python website/check_translations.py` (lancé par la CI) vérifie qu'aucune ne manque.

Si ton clone est dans un dossier synchronisé, fais pointer `PLATFORMIO_BUILD_DIR` vers un dossier en dehors. Les compilations écrivent des centaines de mégaoctets dans `firmware/.pio/`.

## Travailler avec un agent IA

Tu peux utiliser n'importe quel agent de code. Les instructions du projet sont dans [`AGENTS.md`](../AGENTS.md), la convention commune à tous les outils, et tout ce qu'il référence est du Markdown simple.

| Outil | Comment il prend en compte `AGENTS.md` |
|-------|----------------------------------------|
| Claude Code | Automatiquement. Il charge aussi le plugin `mattpocock-skills` (`/tdd`, `/triage`, …). |
| OpenAI Codex, Cursor, GitHub Copilot (VS Code), Zed, Jules | Automatiquement. |
| Aider | Via le `.aider.conf.yml` versionné. |
| Gemini CLI | Mets `"contextFileName": "AGENTS.md"` dans `.gemini/settings.json` (local, ou dans tes réglages utilisateur). |
| Tout autre outil | Dis-lui de lire `AGENTS.md` en premier. |

Les skills en commande slash sont propres à Claude Code. Les agents qui ne les ont pas suivent les mêmes conventions en lisant `docs/agents/`, `CONTEXT.md` et les ADR.

## Comment le travail avance

1. Choisis un ticket ouvert qui n'a aucun bloqueur ouvert. Les tickets marqués `ready-for-agent` sont entièrement spécifiés.
2. Crée une branche depuis `main`, implémente en écrivant les tests d'abord quand le Robot core est concerné, et ouvre une PR qui référence le ticket.
3. La CI doit passer. Le comportement que les tests ne peuvent pas couvrir va dans la checklist de recette physique.
4. Si tu fixes un nouveau terme ou une nouvelle décision, mets à jour `CONTEXT.md` ou ajoute un ADR dans la même PR.
5. Si tu modifies une page utilisateur en anglais, mets à jour sa page française (`.fr.md`) dans la même PR.

Le matériel nécessaire pour tester sur le robot : une carte ESP32, ESP32-S3 ou ESP32-C3, deux servos MG996R, la chaîne d'alimentation USB-C du [guide de câblage](wiring.fr.md#alimentation) (ou une alimentation 5 V de 3 A ou plus), et un mécanisme Mk3 imprimé.

**Les cartes ESP8266 ne sont pas compatibles, et ne le seront presque certainement jamais.** Voir l'[ADR 0008](adr/0008-no-esp8266-support.md) (en anglais).

## Obtenir de l'aide

Les questions vont sur le [serveur Discord](https://discord.gg/Rbqge3v9dk) ; les bugs dans les [issues GitHub](https://github.com/vistorx/CatWandConnect/issues/new/choose). Voir [Obtenir de l'aide](../SUPPORT.fr.md).
