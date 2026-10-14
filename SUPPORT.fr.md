# Obtenir de l'aide

CatWandConnect est un projet gratuit, maintenu sur du temps libre. L'aide est publique et au mieux : il n'y a **aucun délai de réponse garanti**, mais chaque signalement est lu. Le français et l'anglais sont tous deux bienvenus ; on te répond dans ta langue quand c'est possible.

## Où aller

| Tu as… | Va sur |
|---|---|
| Une question, un problème de montage, ou envie de montrer ton robot | Le [serveur Discord](https://discord.gg/Rbqge3v9dk) : poste dans `#support` (un post par problème) ou `📸・showroom`. Pas besoin de compte GitHub. |
| Un bug, avec un compte GitHub | [Ouvre une issue](https://github.com/vistorx/CatWandConnect/issues/new/choose) et choisis le formulaire qui correspond. |
| Un bug, sans compte GitHub | Sur ton robot : Réglages → Robot → Aide → Signaler un problème → **Envoyer au support**. Le rapport de diagnostic anonymisé du robot devient une issue GitHub publique, créée pour toi. Ou poste-le dans `#support` sur [Discord](https://discord.gg/Rbqge3v9dk) : les bugs confirmés y sont transformés en issues GitHub pour toi. |
| Une faille de sécurité | **Jamais en public.** Signale-la en privé : voir [SECURITY.md](SECURITY.md) (en anglais). |

Avant de demander, le [guide de dépannage](docs/troubleshooting.fr.md) répond aux problèmes courants : la carte n'apparaît pas à l'installation, la configuration du Wi-Fi, les mouvements, les mises à jour et le code PIN des réglages.

## Quoi fournir

Plus tu en donnes, plus on peut t'aider vite :

- **Le rapport de diagnostic.** Dans l'interface du robot, ouvre Réglages → Infos, puis **Copier le rapport** ou **Télécharger le rapport**, et colle-le ou joins-le. Il contient la version du firmware, la puce, la raison du dernier redémarrage, les réglages et le log récent, avec le nom de ton Wi-Fi et tes adresses masqués. Les firmwares antérieurs à 0.1.0-beta.2 n'ont pas de rapport : appuie plutôt sur **Copier** sous la liste des Infos.
- **Ce que tu as fait, ce que tu attendais, et ce qui s'est passé à la place.**
- **Ton montage :** la carte et la puce (ESP32, ESP32-S3 ou ESP32-C3), le mécanisme (par exemple Mk2), et comment tu as installé le firmware (flasher web, mise à jour, compilé depuis les sources).
- **Le log série, si tu peux.** Ouvre le [flasher web](https://vistorx.github.io/CatWandConnect/flasher/), connecte la carte, puis **Logs & Console**. Copie les lignes autour du problème.

Vérifie que ce que tu colles ne contient ni ton mot de passe Wi-Fi ni ton mot de passe MQTT : les issues et les posts Discord sont publics.

## Ce qu'on ne prend pas en charge

- **Les cartes ESP8266.** Elles ne sont pas compatibles, et ne le seront presque certainement jamais : voir l'[ADR 0008](docs/adr/0008-no-esp8266-support.md) (en anglais).
- **Les firmwares modifiés.** On aide sur les versions publiées. Si tu as modifié le code, dis-le et joins tes modifications.
