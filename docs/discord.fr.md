# Serveur Discord

Le [serveur Discord CatWandConnect](https://discord.gg/Rbqge3v9dk) est l'endroit où l'on pose ses questions, où l'on trouve de l'aide pour son montage et où on le partage, sans avoir besoin de compte GitHub. Cette page décrit son organisation, pour les mainteneurs. Les utilisateurs n'ont besoin que de [Obtenir de l'aide](../SUPPORT.fr.md).

## Invitation

`https://discord.gg/Rbqge3v9dk` est l'invitation permanente (sans expiration ni limite d'utilisation) ; elle arrive dans `#welcome`. N'utilise que ce lien dans le firmware, le flasher web et la documentation. Pour vérifier qu'une invitation n'a pas été remplacée par une invitation qui expire :

```sh
curl -s "https://discord.com/api/v10/invites/Rbqge3v9dk?with_expiration=true"
```

`expires_at` doit valoir `null`.

## Salons

| Catégorie | Salon | Type | Rôle |
|---|---|---|---|
| Information | `#welcome` | Texte, lecture seule | Message d'accueil épinglé : flasher web, où trouver de l'aide, signalement de sécurité. |
| Information | `#announcements` | Texte, lecture seule | Versions du firmware, publiées automatiquement par GitHub. |
| Support | `#support` | Forum | Un post par problème ou question. Tags : `bug`, `installation`, `question`, `idea`, alignés sur les labels GitHub. |
| Community | Un salon par langue | Texte | Discussion dans la langue des membres, nommé `<drapeau>・<langue>` avec un sujet écrit dans cette langue. Seulement les langues que les mainteneurs peuvent modérer. |
| Community | `📸・showroom` | Forum, galerie | Un post par montage, avec photos ou vidéo. Tags : `Mk3`, `custom build`, `ESP32`, `ESP32-S3`, `ESP32-C3`, `video`. |

Lecture seule signifie que `@everyone` n'a pas les permissions Envoyer des messages, Envoyer des messages dans les fils et Créer des fils publics.

## De Discord à GitHub

Un post `#support` qui s'avère être un bug confirmé devient une issue GitHub qui renvoie vers le post, et le post reçoit le lien de l'issue. Les questions restent sur Discord. Voir l'[ADR 0009](adr/0009-support-flows-end-in-github-issues.md) (en anglais).

## Annonces des versions

GitHub publie chaque version dans `#announcements` via un webhook Discord :

1. Dans `#announcements` : Modifier le salon → Intégrations → Webhooks → Nouveau webhook → Copier l'URL du webhook.
2. Dans le dépôt : Settings → Webhooks → Add webhook. Payload URL est l'URL copiée suivie de `/github`, content type `application/json`, et seulement l'événement **Releases**.

L'URL du webhook est un secret : quiconque la possède peut poster dans le salon. Ne la colle jamais dans une issue, un commit ou une discussion. Si elle fuite, supprime le webhook dans Discord et crée-en un nouveau.

## Administration

Le serveur est administré par une application bot privée (« CatWandConnect Admin », permission Administrateur, **Public Bot** désactivé, son rôle en haut de la liste des rôles) via l'API REST de Discord. Son token est gardé hors du dépôt, sur la machine du mainteneur. Le relais de support utilise un bot distinct, **CatWandConnect Support**, avec seulement les permissions dont il a besoin. Son token n'existe que dans les secrets Actions du dépôt. Ce bot ouvre les posts `#support` des rapports envoyés depuis les robots, répond à la commande `/issue` des modérateurs, et poste dans les posts liés quand un correctif sort ([Relais de support](support-relay.md#discord), en anglais).
