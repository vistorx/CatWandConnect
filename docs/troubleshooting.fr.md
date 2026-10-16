# Dépannage

Quand quelque chose ne va pas, ouvre d'abord le moniteur série. Branche la carte en USB et lance `pio device monitor` depuis `firmware/` (115200 bauds). Le robot écrit dans le log ce qu'il fait, par exemple `[wifi] connected, open http://192.168.1.20/`.

## Installer avec le flasher web

**La carte n'apparaît pas dans la liste des ports.**
Suis ces étapes dans l'ordre :

1. **Essaie un autre câble USB.** Beaucoup de câbles ne font que l'alimentation ; il en faut un qui transporte les données. Branche directement sur l'ordinateur, sans hub.
2. **Repère la puce USB.** C'est la petite puce à côté du connecteur USB de la carte ; son inscription indique le pilote nécessaire. Les cartes dont l'USB va directement à un ESP32-S3 ou un ESP32-C3 (aucune puce à côté du connecteur) n'ont besoin d'aucun pilote.
3. **Installe son pilote**, puis débranche et rebranche la carte et rouvre la liste des ports :

   | Inscription de la puce | Windows | macOS |
   |------------------------|---------|-------|
   | **CH340**, CH341 | [CH341SER.EXE](https://www.wch-ic.com/downloads/CH341SER_EXE.html) | [CH34XSER_MAC.ZIP](https://www.wch-ic.com/downloads/CH34XSER_MAC_ZIP.html) |
   | **CH9102**, CH343 | [CH343SER.EXE](https://www.wch-ic.com/downloads/CH343SER_EXE.html) | [CH34XSER_MAC.ZIP](https://www.wch-ic.com/downloads/CH34XSER_MAC_ZIP.html) |
   | **CP2102**, CP2104, CP210x | [Pilotes CP210x VCP](https://www.silabs.com/software-and-tools/usb-to-uart-bridge-vcp-drivers) | même page, paquet macOS |

   Linux et ChromeOS incluent ces pilotes. Sur macOS, autorise le pilote dans **Réglages Système → Confidentialité et sécurité** après l'avoir installé.
4. **Toujours rien ?** Maintiens le bouton **BOOT** de la carte en la branchant, puis réessaie. Le guide d'Espressif pour [vérifier la connexion série](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/establish-serial-connection.html) (en anglais) aide à trouver le port à la main.

**« Failed to initialize » ou l'installation s'arrête en cours de route.**
Ferme tout autre programme qui utilise le port (moniteur série, Arduino IDE, un autre onglet du navigateur), maintiens **BOOT**, et appuie de nouveau sur Connecter et installer.

## Wi-Fi

**Je ne trouve pas le hotspot `CatWandConnect-XXXX`.**
Le hotspot n'apparaît que quand le robot ne peut joindre aucun Wi-Fi connu. S'il a rejoint ton réseau, ouvre plutôt son adresse `.local` (affichée à la fin de la configuration, par exemple `http://catwandconnect-3f9a.local/`). Après une tentative ratée, le hotspot peut mettre jusqu'à 20 s à apparaître.

**La page de configuration ne s'ouvre pas quand je me connecte au hotspot.**
Va sur `http://192.168.4.1/`. Certains téléphones demandent aussi s'il faut « rester connecté » à un réseau sans internet : réponds oui.

**« Connexion impossible » sur la page de configuration.**
Vérifie le nom du réseau et le mot de passe. L'ESP32 ne gère que le Wi-Fi **2,4 GHz** ; si ta box a des réseaux 2,4 GHz et 5 GHz séparés, choisis le 2,4 GHz.

**Comment faire passer le robot sur un autre réseau Wi-Fi ?**
Ouvre **Réglages → Réseau** : le bloc Wi-Fi affiche le réseau actuel, l'adresse IP du robot et son adresse `.local`. Touche **Changer de réseau Wi-Fi**, lis l'avertissement, puis choisis le nouveau réseau et saisis son mot de passe. Une fois qu'il l'a rejoint, mets ton téléphone sur ce réseau et ouvre l'adresse affichée. S'il n'y arrive pas (mauvais mot de passe, réseau 5 GHz, hors de portée), il revient sur le réseau actuel en une minute environ et rien ne change ; si ça échoue aussi, son hotspot `CatWandConnect-XXXX` s'ouvre, comme à la première installation. Avec un code PIN de réglages, le changement le demande.

**L'adresse `.local` du robot ne s'ouvre pas.**
Vérifie l'adresse exacte : elle est affichée à la fin de la configuration, dans les Réglages et sur la page Infos. Avec le nom par défaut, elle se termine par 4 caractères propres au robot, par exemple `catwandconnect-3f9a.local` (les robots installés avant la version 0.2.1 gardent `catwandconnect.local`). Si tu as renommé le robot, son adresse a changé aussi, par exemple `catwandconnect-salon.local`.
Certains téléphones Android et d'anciens PC Windows ne résolvent pas les noms en `.local`. Utilise l'adresse IP affichée à la fin de la configuration ou dans Réglages → Réseau, ou trouve-la dans la liste des appareils de ta box ou dans le moniteur série. Vérifie aussi que ton téléphone est sur le même Wi-Fi que le robot, pas en données mobiles ni sur un réseau invité.
Ouverte par son adresse IP, la page du robot vérifie si ce téléphone ou cet ordinateur arrive à ouvrir l'adresse `.local` : si oui, elle propose d'y aller ; sinon, elle le signale et propose la suite. Donne alors au robot une adresse fixe dans ta box (une *réservation DHCP*, en général dans les appareils connectés ou les réglages du réseau local) pour que l'adresse IP, et l'icône de l'écran d'accueil qui l'utilise, continuent de marcher.

**Le robot repasse sans cesse sur son hotspot.**
Le signal est trop faible ou la box a changé. Rapproche le robot, ou saisis le nouveau mot de passe sur la page de configuration du hotspot. Tant que personne n'est connecté au hotspot, le robot réessaie ton réseau toutes les 2 minutes.

## Mouvements

**Les servos ne bougent pas du tout.**
- Vérifie la masse commune entre la carte et l'alimentation des servos.
- Vérifie que les fils de signal sont sur les broches indiquées dans Réglages → Matériel → Câblage (les valeurs par défaut sont dans [Câbler l'électronique](wiring.fr.md#broches-par-défaut)).
- Au démarrage, le robot rejoint très lentement (10 °/s) sa position de repos, puis relâche les servos. C'est normal.

**Les servos tremblent ou la carte redémarre quand ils bougent.**
L'alimentation est trop faible ou les fils trop fins. Utilise un chargeur USB-C de 30 W ou plus (45 W c'est mieux) avec un convertisseur abaisseur de 5 A ou plus, ou une alimentation 5 V de 3 A ou plus, et ajoute un condensateur de 470–1000 µF entre les rails +5 V et GND. Voir [Alimentation](wiring.fr.md#alimentation).

**Le joystick ne fait rien.**
- Vérifie que le coin en haut à droite indique « Connecté ».
- Si le bouton Ranger indique « Rangement… », le robot est en train de se ranger, et rien n'interrompt le rangement. Attends qu'il ait fini.

**La baguette s'arrête dans une plage étroite.**
Tant que la calibration n'a pas été faite, les mouvements sont limités à 60°–120° sur chaque axe, par sécurité. Lance-la depuis le bandeau de l'écran principal ou depuis les Réglages.

**La calibration refuse une position.**
Le maximum d'un axe doit être au-dessus de son minimum, et la position de repos doit être dans les limites que tu viens de régler. Ajuste encore et valide de nouveau.

**La baguette bouge dans le mauvais sens.**
Le plus simple : lance la calibration et coche **Inverser cet axe** à l'étape du minimum de cet axe ; le bras ne bouge pas et rien ne redémarre. Ou coche **Inverser le sens** dans Réglages → Matériel → Câblage, puis relance la calibration.

**J'ai choisi les mauvaises broches et les servos ne bougent pas.**
Recâble sur les broches indiquées dans Réglages → Matériel → Câblage, ou choisis les broches que tu as câblées. Si tu ne peux plus ouvrir l'interface, efface et reflashe la carte (`pio run -e <chip> -t erase`, puis téléverse) pour revenir aux valeurs par défaut.

## Mises à jour du firmware

**Sur le hotspot du robot, je ne peux pas choisir le fichier.**
Les téléphones ouvrent les pages des hotspots dans une fenêtre de connexion Wi-Fi qui n'a pas de sélecteur de fichier. Reste sur le hotspot, ouvre `http://192.168.4.1/settings.html#robot` dans ton navigateur habituel (Chrome, Safari…), et envoie le fichier depuis là. Sur ton Wi-Fi habituel, ce problème n'existe pas.

**« Ce firmware est prévu pour une autre puce ».**
Chaque puce a son propre fichier. La page Infos indique ta puce (ESP32, ESP32-S3 ou ESP32-C3) ; télécharge le fichier `-ota.bin` qui porte ce nom.

**La mise à jour a échoué ou la page a perdu la connexion.**
Le robot garde son firmware actuel : le nouveau n'est utilisé qu'une fois reçu entièrement et vérifié. Recharge la page et réessaie, plus près de la box si le Wi-Fi est faible.

**Le robot ne revient pas après une mise à jour.**
Reflashe-le en USB avec le flasher web (ou PlatformIO). Les réglages sont conservés, sauf si tu choisis d'effacer.

## Code PIN des réglages

**J'ai oublié le code PIN.**
Il ne peut pas être récupéré par le réseau. Efface et reflashe le robot (`pio run -e <chip> -t erase`, puis téléverse). Tout revient aux réglages d'usine, y compris le Wi-Fi et la calibration.

**« Trop de codes incorrects ».**
Après 5 codes faux, le robot refuse les nouveaux essais pendant 30 s, puis plus longtemps après chaque nouvel échec, jusqu'à 15 minutes. Attends et réessaie. Jouer et ranger fonctionnent toujours en attendant.

## Le robot ne répond plus

**La page indique « Reconnexion… » ou « Besoin d'aide ? » et rien ne s'ouvre.**
Ton téléphone ne joint plus le robot, donc sa propre page Aide ne peut pas s'ouvrir non plus.

- Vérifie que le robot est alimenté : un chargeur USB ou un port d'ordinateur qui fournit assez de courant, et un câble qui transporte l'alimentation.
- Vérifie que ton téléphone est sur le même Wi-Fi que le robot, pas en données mobiles ni sur un réseau invité.
- Si le robot a perdu ton Wi-Fi, il ouvre son hotspot `CatWandConnect-XXXX` : connecte-toi et ouvre `http://192.168.4.1/` (voir [Wi-Fi](#wi-fi)).
- Débranche le robot quelques secondes et rebranche-le. La page se reconnecte toute seule.
- Toujours rien : branche-le sur un ordinateur et ouvre « Logs & Console » sur le [flasher web](https://vistorx.github.io/CatWandConnect/flasher/) pour lire ce qu'il dit, puis demande sur [Discord](https://discord.gg/Rbqge3v9dk) ou [ouvre une issue](https://github.com/vistorx/CatWandConnect/issues/new/choose) avec ce log.

## Toujours bloqué ?

Voir [Obtenir de l'aide](../SUPPORT.fr.md) : demande sur le [serveur Discord](https://discord.gg/Rbqge3v9dk), sans compte GitHub, ou [ouvre une issue](https://github.com/vistorx/CatWandConnect/issues/new/choose). Joins le **rapport de diagnostic** de la page Infos (Réglages → Robot → Ouvrir Info → Copier le rapport) : il contient déjà le log récent du robot, avec le nom de ton Wi-Fi et tes adresses masqués. Les problèmes de sécurité se signalent [en privé](../SECURITY.md), jamais en public.
