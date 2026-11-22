# Accès à distance

L'accès à distance ouvre la page du robot quand tu n'es pas chez toi : vérifie qu'il va bien, lance une session de jeu ou range-le depuis le travail ou en vacances. Il est facultatif et désactivé tant que tu ne l'actives pas.

Le robot utilise [Tailscale](https://tailscale.com/), un réseau privé gratuit pour tes propres appareils (un *tailnet*). Le robot rejoint ton tailnet comme un appareil à part entière, et ton téléphone le rejoint avec l'application Tailscale. Seuls tes appareils peuvent joindre le robot ; rien n'est ouvert sur ta box et aucun service CatWandConnect ne passe au milieu.

## Ce qu'il te faut

| Quoi | Remarques |
|------|-------|
| Un compte Tailscale | Gratuit pour un usage personnel. Inscris-toi sur [tailscale.com](https://tailscale.com/) avec un compte Google, Microsoft, Apple ou GitHub. |
| L'application Tailscale sur ton téléphone | Sur l'App Store ou Google Play, connectée au même compte. |
| Le robot sur ton Wi-Fi | Un ESP32 classique, un ESP32-S3 ou un ESP32-C3 avec un firmware qui a l'accès à distance. |

## L'activer

1. Dans ton compte Tailscale, ouvre la [page des clés](https://login.tailscale.com/admin/settings/keys) et clique sur **Generate auth key**. Laisse **Reusable** désactivé, et mets l'expiration la plus longue possible (90 jours au plus) : Tailscale ne demande une clé que pour la première connexion du robot, ensuite le robot garde sa propre clé d'appareil.
2. Copie la clé. Elle commence par `tskey-auth-`.
3. Sur la page du robot, ouvre **Réglages → Réseau → Accès à distance**, coche **Activé**, colle la clé et appuie sur **Enregistrer** (saisis le code PIN des réglages s'il y en a un).
4. **État** passe de *Connexion…* à *Connecté*, en moins d'une minute. **Adresse à distance** affiche l'adresse du robot sur le tailnet, par exemple `http://100.101.102.103/`.
5. Dans la [console d'administration Tailscale](https://login.tailscale.com/admin/machines), trouve le robot (nommé comme son adresse `.local`, par exemple `catwandconnect-3f9a`), ouvre son menu **⋯** et choisis **Disable key expiry**, sinon le robot quitte le tailnet quand sa clé expire. **La clé expire le** dans les réglages affiche la date tant que tu ne l'as pas fait.

Quand tu n'es pas chez toi, ouvre l'application Tailscale sur ton téléphone, vérifie qu'elle est connectée, et ouvre l'**Adresse à distance** du robot dans ton navigateur. Ajoute-la aussi à ton écran d'accueil.

!!! note "En direct ou via un relais"
    *Connecté (direct)* veut dire que ton téléphone a joint le robot directement par internet. *Connecté (via un relais)* veut dire que ça passe par un des relais de Tailscale, ce qui marche partout mais un peu plus lentement. Dans les deux cas, c'est chiffré de bout en bout.

## Le désactiver

Décoche **Activé** et appuie sur **Enregistrer**. Le robot quitte ton tailnet tout de suite et oublie sa clé et ses clés d'appareil. Supprime aussi le robot dans la console d'administration Tailscale. Pour réutiliser l'accès à distance, colle une nouvelle clé.

## Bon à savoir

- **Une nouvelle clé.** Si l'état indique *Clé expirée : colle une nouvelle clé*, crées-en une comme plus haut et colle-la : le robot quitte ton tailnet et le rejoint avec elle.
- **Une mise à jour du firmware à distance demande deux confirmations.** Si la mise à jour se passe mal, personne ne sera peut-être près du robot pour le rallumer. Tu peux quand même mettre à jour ; la page te demande de confirmer une deuxième fois.
- **La clé reste sur le robot.** Elle est enregistrée dans la mémoire flash du robot comme ton mot de passe Wi-Fi, et n'est plus jamais affichée, ni dans le log du robot, ni dans le rapport de diagnostic. Le rapport masque aussi l'adresse et le nom du robot sur le tailnet.
- **À quoi le robot se connecte.** Quand l'accès à distance est activé, et seulement à ce moment-là, le robot se connecte aux serveurs de Tailscale pour trouver tes appareils, et à tes appareils. Quand il est désactivé, il ne se connecte à rien en dehors de chez toi ([ADR 0015](adr/0015-the-robot-connects-to-the-owners-remote-access-provider.md)).
- **Petits tailnets.** Le robot a peu de mémoire : il est fait pour un tailnet personnel avec quelques appareils.

## S'il ne se connecte pas

| État | Que faire |
|------|-------|
| *Connexion…* pendant plus de 2 minutes | Vérifie que le Wi-Fi du robot a accès à internet. Certains réseaux invités ou d'école bloquent Tailscale. |
| *Clé expirée : colle une nouvelle clé* | La clé a déjà servi, a expiré ou a été révoquée. Crées-en une nouvelle et colle-la. |
| *Erreur, nouvel essai en cours* | Le robot réessaie toutes les 30 secondes. Si ça dure, [ouvre une issue](https://github.com/vistorx/CatWandConnect/issues/new/choose) avec le rapport de diagnostic de la page Infos. |
| *Non disponible sur cette puce* | Ce firmware n'a pas de client Tailscale pour ta carte. |
