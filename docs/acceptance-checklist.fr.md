# Checklist de recette physique

Les vérifications qui demandent le vrai robot, à faire avant chaque version sur un mécanisme Mk3. Les tests automatiques couvrent le Robot core ; tout ce qui est ici concerne le matériel et le réseau autour. Note la carte, la puce et la version du firmware utilisées. Chaque ticket qui ajoute un comportement que les tests ne couvrent pas ajoute sa section ici.

Les textes entre guillemets sont ceux de l'interface en anglais ; avec l'interface en français, cherche leur équivalent (par exemple « Stow » s'affiche « Ranger »).

Garde un moniteur série ouvert (`pio device monitor` dans `firmware/`) pour lire les lignes de log `[wifi]`.

## Configuration du Wi-Fi

Commence chaque section avec une carte sans Wi-Fi enregistré : efface-la avec `pio run -e <chip> -t erase`, puis flashe-la.

### Improv par USB

- [ ] Flashe la carte avec le flasher web (ou avec PlatformIO, puis ouvre le flasher web et connecte-toi à la carte). Il propose « Connect to Wi-Fi » et affiche l'appareil comme CatWandConnect avec la bonne version.
- [ ] Le client liste les réseaux proches.
- [ ] Un mauvais mot de passe affiche « unable to connect », et le robot ouvre son hotspot.
- [ ] Les bons identifiants connectent le robot, et le client affiche un lien vers l'adresse du robot. Le lien ouvre la page du robot.
- [ ] Redémarrer la carte (débrancher et rebrancher) la reconnecte sans redemander.

### Hotspot et page de configuration

- [ ] Sans Wi-Fi enregistré, un réseau `CatWandConnect-XXXX` apparaît quelques secondes après la mise sous tension.
- [ ] S'y connecter depuis un téléphone ouvre automatiquement la page de configuration, sur iOS comme sur Android. Sinon, `http://192.168.4.1/` l'ouvre.
- [ ] La page de configuration liste les réseaux proches, du plus fort au plus faible, le premier présélectionné, avec un cadenas sur les réseaux sécurisés. Toucher un réseau remplit son nom ; « Rescan » relance la recherche ; un réseau masqué peut toujours être saisi.
- [ ] Sur la page de configuration, « Continue without Wi-Fi » ouvre l'écran principal via le hotspot ; le joystick et les Réglages fonctionnent, et Réglages → Réseau → Wi-Fi → « Connect to Wi-Fi » ramène la page de configuration.
- [ ] Un mauvais mot de passe affiche « Could not connect » et le hotspot reste actif.
- [ ] Le bon mot de passe affiche « Connected » avec l'adresse du robot. Le hotspot disparaît environ 30 s plus tard.
- [ ] Un redémarrage reconnecte à ce réseau sans hotspot.

### Changer de réseau Wi-Fi (depuis les Réglages)

Il faut un second réseau 2,4 GHz (le partage de connexion d'un téléphone convient).

- [ ] Sur le Wi-Fi habituel, Réglages → Réseau → Wi-Fi affiche le nom du réseau, l'adresse IP (la même que sur la page Infos) et `http://<hostname>.local/`, avec « Change Wi-Fi network ». « Connect to Wi-Fi » n'apparaît pas.
- [ ] « Change Wi-Fi network » affiche l'avertissement (l'adresse peut changer, retour au réseau actuel, puis hotspot) avec Continuer et Annuler ; Annuler le masque.
- [ ] Avec un code PIN de réglages, se connecter depuis la page de configuration demande d'abord le PIN ; Annuler laisse le robot sur son réseau.
- [ ] Continuer ouvre la page de configuration qui dit sur quel réseau est le robot, sans « Continue without Wi-Fi », avec le réseau le plus fort autre que l'actuel présélectionné, et « Back to Settings ».
- [ ] Un mauvais mot de passe pour le second réseau : la page indique le retour, puis « back on <réseau habituel>, nothing changed » en une minute environ ; le journal série affiche `back to the saved network` ; le robot répond à son adresse habituelle et aucun hotspot n'apparaît.
- [ ] Le bon mot de passe : le robot rejoint le second réseau (journal série `connected`) ; une fois sur ce réseau, le téléphone l'ouvre à l'IP affichée et à `<hostname>.local`. Home Assistant (MQTT ou intégration native) s'y reconnecte s'il est joignable.
- [ ] Un redémarrage garde le second réseau. Revenir au réseau habituel de la même façon fonctionne.
- [ ] Box éteinte juste après un mauvais mot de passe : le robot ouvre son hotspot `CatWandConnect-XXXX` après la tentative de retour (environ 40 s), et s'y connecter ramène la page de configuration.

### Nom et reprise

- [ ] Depuis un ordinateur ou un téléphone sur le réseau de la maison, l'adresse `.local` affichée à la fin de la configuration (`http://catwandconnect-XXXX.local/` avec le nom par défaut) ouvre la page du robot.
- [ ] Deux robots fraîchement flashés qui gardent le nom par défaut, sur le même réseau, s'ouvrent chacun à leur propre adresse `catwandconnect-XXXX.local`.
- [ ] Un robot qui répondait à `catwandconnect.local` avant la mise à jour en 0.2.1 y répond toujours après, et ses entités Home Assistant sont inchangées.
- [ ] Éteindre la box : en une minute environ, le hotspot réapparaît.
- [ ] Rallumer la box : en 2 minutes environ, sans téléphone sur le hotspot, le robot rejoint le réseau de la maison et le hotspot disparaît.
- [ ] Changer le mot de passe de la box : le hotspot apparaît, et le nouveau mot de passe peut être saisi sur la page de configuration.

## Pilotage manuel

Câble les servos sur les broches par défaut : GPIO 18 (horizontal) et 19 (vertical) sur ESP32, GPIO 4 et 5 sur ESP32-S3 et ESP32-C3. Alimente les servos avec leur propre alimentation 5 V, en partageant la masse avec la carte.

- [ ] À la mise sous tension, les deux servos vont en position milieu (90°) et la tiennent.
- [ ] `http://catwandconnect.local/` ouvre l'écran principal sur un téléphone, avec « Connected » et les deux positions.
- [ ] Faire glisser le joystick déplace la baguette en douceur dans la direction voulue. Un petit mouvement la déplace lentement ; un mouvement complet la déplace à pleine vitesse.
- [ ] Les positions à l'écran suivent la baguette.
- [ ] Relâcher arrête la baguette en une fraction de seconde.
- [ ] Joystick à fond dans chaque direction : la baguette s'arrête aux limites de course par défaut (60°–120°) et ne heurte jamais le mécanisme.
- [ ] Pendant un mouvement, couper le Wi-Fi du téléphone ou fermer l'onglet arrête la baguette en une seconde environ.
- [ ] Un second téléphone sur l'écran principal voit aussi les positions bouger.
- [ ] La page est utilisable sur un téléphone en portrait, sans zoomer ni faire sortir le joystick de l'écran.
- [ ] En paysage, le joystick, les positions, le curseur de vitesse et Ranger tiennent à l'écran sans défilement ; faire glisser le joystick ne fait jamais défiler la page.

## Vitesse et réglages

- [ ] Le curseur de vitesse change la vitesse à laquelle le joystick déplace la baguette. À 10 %, la baguette va à la vitesse minimale ; à 100 %, elle atteint la vitesse maximale.
- [ ] Déplacer le curseur sur un téléphone le met à jour sur un second téléphone.
- [ ] Dans les Réglages, baisser la vitesse maximale horizontale ralentit seulement le mouvement horizontal.
- [ ] Baisser l'accélération rend les départs et les arrêts visiblement plus doux ; l'augmenter les rend plus vifs, sans que le mécanisme ne soit secoué ni ne saute.
- [ ] Saisir 5 °/s comme vitesse maximale affiche une erreur et garde la valeur précédente.
- [ ] Régler la vitesse minimale horizontale à 20 °/s rend les 10 % du curseur nettement plus rapides qu'avant ; une minimale égale à la maximale affiche une erreur.
- [ ] Après une mise à jour OTA depuis la 0.1.0, les réglages et la calibration sont conservés, et chaque vitesse minimale vaut le dixième de sa maximale.
- [ ] Réglages → Mouvement → Régler vitesses et accélération, maximum horizontal : la baguette balaie en douceur entre les limites de course et l'axe vertical reste immobile. Plus vite / Plus lent changent la vitesse du balayage en moins d'une seconde ; le nombre affiché correspond à ce que tu vois.
- [ ] Garder cette vitesse l'enregistre (Réglages → Mouvement l'affiche) et range le robot ; Annuler, Ranger depuis un autre téléphone ou depuis Home Assistant, et toucher le joystick terminent tous le réglage en gardant la valeur précédente.
- [ ] Régler la vitesse minimale d'un axe équipé d'un SG90 : à 2–4 °/s, le mouvement saccade visiblement ; la valeur gardée rend les 10 % du curseur fluides.
- [ ] Réglages → Mouvement → Régler vitesses et accélération, accélération horizontale : la baguette fait de courts allers-retours autour du milieu de sa course, avec une pause à chaque bout, et l'axe vertical reste immobile. Plus nerveux rend les départs et les arrêts visiblement plus secs, Plus doux visiblement plus doux ; Garder cette accélération l'enregistre (Réglages → Mouvement → Accélération l'affiche) et range le robot ; Annuler, Ranger et le joystick gardent la valeur précédente.
- [ ] Après une coupure d'alimentation, les valeurs du curseur et des Réglages sont conservées.

## Session de jeu

Utilise le bloc Session de jeu sous le curseur de vitesse de l'écran principal.

- [x] Avant la calibration, Démarrer affiche « Lance d'abord la Calibration » avec un lien Calibrer, et rien ne bouge.
- [x] − et + changent la durée d'une minute, entre 1 et 60. Après un rechargement de la page, et sur un second téléphone, la dernière durée choisie s'affiche.
- [x] Une fois le robot calibré, Démarrer fait bouger les deux axes chacun à son rythme : ils démarrent ou s'arrêtent rarement ensemble, de petits sauts arrivent, et la baguette ne dépasse jamais les limites de course.
- [x] Pendant la session, le bouton affiche Arrêter et le temps restant décompte sans à-coups, seconde par seconde ; un second téléphone affiche la même session et le même temps restant.
- [x] Dans Réglages → Mouvement, règle les deux plages de pause à 0–0 s et enregistre : pendant une session, la baguette ne s'arrête jamais. Un minimum au-dessus du maximum affiche une erreur et garde les valeurs précédentes.
- [x] Une session d'une minute se range toute seule au bout d'une minute ; Arrêter et le bouton Ranger y mettent fin avec un rangement, sans finir le mouvement en cours, et le bloc revient à Démarrer.
- [x] Toucher le joystick pendant une session donne la main aussitôt, sans rangement, et le bloc revient à Démarrer.
- [x] Fermer la page n'arrête pas la session ; la rouvrir affiche Arrêter et le temps restant. Une coupure de courant pendant une session y met fin, et le robot se range doucement au démarrage au lieu de jouer.
- [x] Après une mise à jour OTA depuis une version sans session de jeu, les réglages et la calibration sont conservés, et Réglages → Mouvement affiche les plages de pause par défaut (0–2 s sur les deux axes).

### Journal des sessions de jeu

- [ ] Infos → Journal liste, du plus récent au plus ancien, une session lancée et laissée jusqu'au bout (Démarrée, puis Terminée), une arrêtée avec Arrêter, avec Ranger et avec le joystick (Arrêtée, avec la raison), et un Démarrer pendant un rangement (Refusée, occupé), chacun avec l'heure et « Interface du robot ». Sessions aujourd'hui compte les démarrages.
- [ ] Un deuxième téléphone avec Infos ouvert affiche un nouvel événement en moins d'une seconde.
- [ ] Après une coupure de courant, les événements sont toujours là, ceux d'avant le redémarrage avec leur heure ; les nouveaux affichent « Il y a … min, avant la mise à l'heure » seulement jusqu'à l'ouverture d'une page, puis leur heure.
- [ ] Le rapport de diagnostic a une section « Play session journal » avec l'état de l'horloge et les événements, sans nom de fuseau horaire.
- [ ] Avec le téléphone en anglais, le Journal est en anglais.

## Rangement, priorité, maintien et relâchement

- [ ] Le bouton Ranger reste visible en bas de l'écran principal, y compris sur un petit téléphone.
- [ ] Appuyer sur Ranger quand la baguette est ailleurs la ramène à la position de repos. Le bouton affiche « Stowing… » jusqu'à son arrivée.
- [ ] À la position de repos, les servos deviennent mous : le bras se déplace à la main avec peu de résistance. Note si le Mk3 reste en place sans couple (voir la note de la spec sur le maintien et le relâchement).
- [ ] Pousser le joystick pendant le rangement ne fait rien ; la baguette atteint quand même la position de repos.
- [ ] Après le rangement, le joystick refonctionne et les servos tiennent fermement pendant le mouvement et après avoir relâché le joystick hors de la position de repos.
- [ ] `curl -X POST http://catwandconnect.local/api/stow` range la baguette.

## Mise sous tension

- [ ] Éloigne la baguette de la position de repos avec le joystick, relâche et attends 2 s, puis débranche l'alimentation. Au rebranchement, les servos ne sautent pas : la baguette repart d'où elle était et rejoint lentement la position de repos, puis les servos deviennent mous.
- [ ] Recommence, mais coupe l'alimentation pendant que la baguette bouge. Au démarrage, elle repart de la dernière position où elle s'est arrêtée : au pire un petit saut, puis elle rejoint lentement la position de repos.
- [ ] Sur une carte tout juste effacée (`pio run -e <chip> -t erase`), la mise sous tension va directement à la position de repos.
- [ ] Pendant que le robot rejoint lentement la position de repos après la mise sous tension, l'écran principal affiche « Stowing… » et le joystick ne fait rien.

## Calibration

- [ ] Sur un robot jamais calibré, l'écran principal affiche le bandeau « Not calibrated yet », et le joystick est limité à 60°–120°.
- [ ] L'assistant enchaîne minimum horizontal, maximum horizontal, minimum vertical, maximum vertical, position de repos, balayage, confirmation, puis vitesse minimale horizontale, vitesse maximale horizontale, accélération horizontale, vitesse minimale verticale, vitesse maximale verticale, accélération verticale et « Your robot is ready », en affichant « Step n of 12 ».
- [ ] À partir de la deuxième étape, « Marked so far » affiche les valeurs déjà validées ; l'étape de la position de repos affiche la plage autorisée de chaque axe ; un maximum en dessous du minimum cite le minimum dans l'erreur.
- [ ] Les boutons ±1° et ±5° ne déplacent que l'axe de l'étape, lentement, et la position à l'écran suit.
- [ ] Les ajustements peuvent dépasser 60°–120°.
- [ ] Chaque étape de minimum ou de maximum indique à quelle position du joystick elle correspond (minimum horizontal = tout à gauche, maximum = tout à droite ; minimum vertical = tout en bas, maximum = tout en haut).
- [ ] À l'étape du minimum horizontal, la case « Reverse this axis » montre le câblage actuel (cochée quand Réglages → Matériel → Câblage indique un sens inversé). Si − envoie le bras à droite, la cocher laisse le bras où il est, reprend l'axe à son minimum, et − l'envoie maintenant à gauche. Le robot ne redémarre pas ; Réglages → Matériel → Câblage montre l'axe inversé, toujours après une coupure d'alimentation, et le joystick déplace cet axe dans le bon sens.
- [ ] Valider un maximum inférieur ou égal à son minimum affiche une erreur et reste sur l'étape.
- [ ] Valider une position de repos en dehors des nouvelles limites affiche une erreur et reste sur l'étape.
- [ ] Le balayage parcourt lentement les quatre coins des nouvelles limites et se termine à la position de repos, sans rien toucher.
- [ ] « Nothing touched: save » enregistre ; le bandeau disparaît et le joystick atteint maintenant les nouvelles limites, mais pas au-delà.
- [ ] Après l'enregistrement, le robot se range, puis l'étape de vitesse minimale horizontale balaie cet axe tout seul entre les nouvelles limites ; Plus vite / Plus lent changent la vitesse et « Keep this speed » l'enregistre et passe à la vitesse suivante.
- [ ] Sur une étape de vitesse, « Skip (keep N °/s) » garde cette vitesse telle quelle (Réglages → Mouvement affiche la même valeur) et passe à la suite.
- [ ] Les étapes de vitesse maximale démarrent le balayage à la vitesse maximale actuelle.
- [ ] Chaque étape d'accélération fait de courts mouvements à la vitesse maximale qui vient d'être gardée ; « Snappier » / « Softer » changent visiblement les départs et les arrêts ; « Keep this acceleration » l'enregistre et passe à la suite ; « Skip (keep N °/s²) » la garde telle quelle.
- [ ] Ranger depuis l'écran principal pendant une étape de vitesse arrête le balayage sans enregistrer ; « Sweep again » le relance.
- [ ] Quitter l'assistant avec Retour pendant une étape de vitesse arrête le balayage ; les nouvelles limites restent enregistrées, ainsi que les vitesses déjà gardées.
- [ ] « Your robot is ready » affiche la plage de vitesses et l'accélération de chaque axe ; Done ramène à l'écran principal.
- [ ] Les Réglages affichent les nouvelles limites et la position de repos. Les y modifier est validé de la même façon.
- [ ] Appuyer sur Annuler à n'importe quelle étape, ou sur Ranger sur l'écran principal, garde les limites précédentes et range le robot.
- [ ] Après une coupure d'alimentation, le robot est toujours calibré, avec les mêmes limites.
- [ ] Relancer la calibration part des limites actuelles.
- [ ] Une fois une calibration enregistrée (y compris en rouvrant l'assistant juste après en avoir terminé une), le bouton de l'assistant indique « Recalibrate » ; pendant une nouvelle calibration, le bouton d'annulation indique « Cancel the new Calibration », garde les limites enregistrées, range le robot et revient à l'écran principal.

## Configuration matérielle

- [ ] Les Réglages s'ouvrent sur l'onglet Mouvement et ont quatre onglets (Mouvement, Matériel, Réseau, Robot) qui tiennent sur un écran de téléphone ; chaque section est dans un onglet, et recharger la page garde l'onglet.
- [ ] Réglages → Matériel → Câblage ne liste que les broches autorisées sur la puce de la carte, et affiche le câblage actuel.
- [ ] Choisir la même broche pour les deux servos affiche une erreur et n'enregistre rien.
- [ ] Recâble le servo horizontal sur une autre broche autorisée, sélectionne-la et enregistre : le robot redémarre, et après le démarrage le servo fonctionne sur la nouvelle broche.
- [ ] Cocher « Reverse direction » sur un axe et enregistrer : le robot redémarre, le joystick déplace cet axe dans l'autre sens, et le bandeau « Not calibrated yet » revient.
- [ ] La plage d'impulsions est sous « Servo travel (advanced) », repliée, avec une explication, les valeurs par défaut et « Reset to default ».
- [ ] Resserrer une plage d'impulsions (par exemple 600–2000 µs) et enregistrer réduit la course de ce servo, et efface la calibration.
- [ ] Une plage d'impulsions de moins de 500 µs affiche une erreur et n'enregistre rien.
- [ ] Le câblage survit à une coupure d'alimentation.

## Identité, Infos, langue et écran d'accueil

- [ ] Dans les Réglages, l'aperçu de l'adresse suit le nom pendant la saisie (« Salon d'été » → `salon-d-ete.local`).
- [ ] Enregistrer un nouveau nom redémarre le robot. Ensuite, le titre de l'écran principal l'affiche, `http://<new-name>.local/` s'ouvre, et l'ancienne adresse ne répond plus.
- [ ] Avec le Wi-Fi du robot oublié, le hotspot s'appelle `<name>-XXXX`.
- [ ] Un nom vide, ou composé uniquement d'emoji, est refusé avec un message.
- [ ] La page Infos affiche le nom, l'adresse, la version du firmware, la puce, le réseau Wi-Fi, l'IP, le signal, la durée de fonctionnement, la température, la mémoire libre et l'adresse MAC, actualisés toutes les quelques secondes. « Copy » les copie, ou les sélectionne si le navigateur refuse.
- [ ] La page Infos affiche un rapport de diagnostic avec la version du firmware, « Last reset: power-on » après avoir débranché et rebranché, les réglages, le câblage et des lignes de log récentes comme `[wifi] connected`.
- [ ] Dans le rapport, le réseau Wi-Fi, les adresses IP et MAC, ainsi que le broker et l'utilisateur MQTT (avec Home Assistant configuré) n'apparaissent que sous la forme `<wifi-network>`, `<ip>`, `<mac>`, `<broker>` et `<mqtt-user>`, y compris dans les lignes de log. Un robot renommé « Salon » affiche `<name>` et `<hostname>`. Le mot de passe MQTT n'apparaît jamais.
- [ ] « Copy the report » le copie, ou le sélectionne si le navigateur refuse ; « Download the report » enregistre un fichier `.txt` sur le téléphone et sur un ordinateur.
- [ ] « Download the full report » enregistre une version avec le vrai nom du Wi-Fi et les vraies adresses, et son nom de fichier se termine par `-full`.
- [ ] Après un redémarrage depuis les Réglages (en renommant le robot), le rapport indique « Last reset: software restart ».
- [ ] Réglages → Robot et la page Infos renvoient vers l'Aide. « Open Discord » ouvre l'invitation du serveur ; « Common problems » ouvre la doc de dépannage.
- [ ] Dans l'Aide, « Prepare the report » avec une description vide en demande une ; avec une description, il affiche le texte public exact (description + rapport anonymisé) et un avertissement indiquant qu'il sera public.
- [ ] Avec le robot débranché pendant 10 s, l'écran principal affiche « Need help? » sous l'en-tête, qui ouvre la section « The robot stopped responding » du dépannage sur GitHub dans un nouvel onglet. Un envoi de firmware raté et une tentative de Wi-Fi ratée affichent « Need help? » qui mène à la page Aide du robot.
- [ ] « Open the issue on GitHub » télécharge le rapport et ouvre le formulaire de bug de GitHub avec la description, le rapport, la version du firmware et la puce déjà remplis.
- [ ] Avec internet sur le téléphone, après « Prepare the report », l'Aide propose « Send to support » avec un champ Contact facultatif. L'envoi affiche « Sent: issue #N was created » et un lien vers cette issue sur GitHub, avec le label `from-device`, contenant la description, l'identifiant d'appareil et le rapport anonymisé complet. Envoyer de la même façon un rapport enregistré sur le hotspot le supprime.
- [ ] Avec « Follow it on Discord » coché et un pseudo Discord dans Contact, l'envoi ouvre aussi un post `#support` avec le tag bug, qui mentionne cet utilisateur et renvoie vers l'issue ; la page affiche « Open the Discord thread », et l'issue renvoie vers le post.
- [ ] Dans un post `#support`, la commande `/issue` d'un modérateur crée une issue `needs-triage` avec le premier message et le rapport joint, et répond avec son lien ; la relancer répond avec la même issue ; un membre sans la permission Gérer les fils est refusé.
- [ ] Après une version qui ferme une issue liée à un post `#support`, le post reçoit une seule fois un message avec la version et la façon de mettre à jour.
- [ ] Avec le téléphone hors ligne ou le service de support désactivé, « Send to support » n'est pas proposé, ou indique que le service est indisponible, et « Open the issue on GitHub » reste disponible.
- [ ] Sur le hotspot de configuration, « A problem? » télécharge le rapport anonymisé ; l'Aide affiche « Save for later » et enregistre une description.
- [ ] Après un redémarrage et une fois sur le Wi-Fi, l'écran principal affiche « A problem report saved on the hotspot is waiting » ; Aide → « Review and send » prépare la même issue, « Discard » la supprime et le bandeau disparaît.
- [ ] Un téléphone réglé en français affiche l'interface en français. Choisir l'anglais dans les Réglages la change immédiatement et le choix est gardé après un rechargement. « Automatique » revient à la langue du téléphone.
- [ ] iPhone (Safari) → Partager → Sur l'écran d'accueil : l'icône apparaît, et l'ouvrir affiche l'interface en plein écran, sans les barres de Safari.
- [ ] Android (Chrome) → menu → Ajouter à l'écran d'accueil : l'icône apparaît et ouvre l'interface.
- [ ] Première visite sur un iPhone dans Safari, sur le Wi-Fi de la maison : une fois l'écran principal chargé, une astuce « Mets le robot sur ton écran d'accueil » montre les étapes Partager → Sur l'écran d'accueil → Ajouter ; « Voir les étapes pour Android » les change. Ouverte par adresse IP, elle propose aussi l'adresse `.local`.
- [ ] Première visite sur un téléphone Android dans Chrome : l'astuce montre les étapes menu ⋮ → Ajouter à l'écran d'accueil (ou Installer l'application) → Ajouter.
- [ ] Après **Compris**, l'astuce ne revient plus, même après des rechargements ; Aide → « Ajouter à ton écran d'accueil » l'affiche quand même. Après **Plus tard** (ou un appui à côté), elle ne revient pas le jour même.
- [ ] Ouverte depuis l'icône de l'écran d'accueil, sur un ordinateur ou sur le hotspot de configuration, l'astuce ne s'affiche jamais.
- [ ] iPhone dans Safari, page du robot ouverte par adresse IP sur le Wi-Fi de la maison : « Utilise l'adresse permanente du robot » montre d'abord l'adresse `http://<nom>.local/` ; **Y aller** ouvre la même page à cette adresse, où l'astuce de l'écran d'accueil s'affiche alors. Rouvrir ensuite la page par IP n'affiche ni l'une ni l'autre.
- [ ] Pareil sur un ordinateur : l'étape de l'adresse permanente s'affiche, et après **Pas maintenant** aucune astuce d'écran d'accueil ne suit.
- [ ] Après **Pas maintenant** sur un téléphone, l'astuce de l'écran d'accueil s'affiche sur la page ouverte par IP avec la note que l'adresse peut changer ; l'étape de l'adresse permanente ne revient pas le jour même.
- [ ] Téléphone Android qui n'ouvre pas les noms en `.local` (ou réseau qui les bloque), page ouverte par IP : pas d'étape d'adresse permanente ; l'astuce de l'écran d'accueil s'affiche avec le conseil de réserver l'adresse du robot dans ta box, et son lien ouvre la doc de dépannage.
- [ ] Ouverte à l'adresse `.local` ou depuis le hotspot de configuration : l'étape de l'adresse permanente ne s'affiche jamais.
- [ ] Réglages → Robot → « Ajouter à ton écran d'accueil » réaffiche l'astuce, même après **Compris**.

## Code PIN des réglages

- [ ] Sur un robot neuf, Réglages → Robot → Code PIN des réglages indique « Off », et tous les réglages se modifient sans code.
- [ ] Définis un code de 4 chiffres. Tu restes connecté sur ce téléphone.
- [ ] Sur un second téléphone (ou une fenêtre privée), le joystick, le curseur de vitesse et Ranger fonctionnent sans code.
- [ ] Sur ce second téléphone, enregistrer n'importe quel réglage (limites, vitesse et accélération, câblage, nom), lancer la calibration ou changer le Wi-Fi depuis le hotspot demande le code. Avec le bon code, la modification passe.
- [ ] Cinq codes faux de suite affichent « Too many wrong PINs » avec un compte à rebours ; le bon code est refusé jusqu'à la fin.
- [ ] Après un redémarrage, le code est redemandé.
- [ ] Changer le code déconnecte les autres téléphones. Le supprimer rouvre tout.
- [ ] Un code de 3 chiffres ou non numérique est refusé.

## Mise à jour du firmware (OTA)

Compile deux versions visiblement différentes (par exemple en changeant `CWC_VERSION` dans `platformio.ini`) et utilise le `firmware.bin` de la seconde.

- [ ] Réglages → Robot → Mise à jour du firmware affiche la version installée.
- [ ] Envoyer le nouveau fichier affiche la progression, puis « Installed ». Le robot redémarre, la page se recharge et la page Infos affiche la nouvelle version.
- [ ] Le Wi-Fi, le nom, la calibration, la vitesse, le câblage et le code PIN sont inchangés après la mise à jour.
- [ ] Envoyer un fichier compilé pour une autre puce est refusé avec « for another chip », et rien ne change.
- [ ] Envoyer un fichier qui n'est pas un firmware (une photo) est refusé avec « not an ESP32 firmware ».
- [ ] Depuis le hotspot (Continuer sans Wi-Fi), Réglages → Robot → Mise à jour du firmware explique d'ouvrir `http://192.168.4.1/settings.html#robot` dans le navigateur habituel ; depuis là, la mise à jour fonctionne.
- [ ] Couper le Wi-Fi du robot (ou fermer l'onglet) en plein envoi : le robot continue avec son ancien firmware, et un nouvel envoi fonctionne ensuite.
- [ ] Avec un code PIN défini, envoyer depuis un téléphone qui ne l'a pas saisi le demande.

### Mise à jour en un geste

Sur un robot qui a une version plus ancienne que la dernière de son canal, avec le téléphone sur Internet.

- [ ] L'écran principal affiche *Version X disponible* avec **Nouveautés** (ouvre les notes de version) et **Mettre à jour**. Sur la dernière version, pas de bandeau, et Réglages → Robot → Mise à jour du firmware dit *Ton robot est à jour*.
- [ ] **Mettre à jour** ouvre Réglages → Robot → Mise à jour du firmware avec la confirmation ; **Installer maintenant** affiche le téléchargement, puis la progression de l'envoi, puis *Le robot est de retour avec la version X*. Le Wi-Fi, le nom, la calibration, la vitesse, le câblage, le code PIN et le Canal de mise à jour sont inchangés.
- [ ] Pendant une Session de jeu ou une Calibration, **Mettre à jour** explique pourquoi il faut attendre, et rien n'est installé.
- [ ] Avec un code PIN défini, **Installer maintenant** le demande avant l'envoi.
- [ ] Téléphone hors ligne (données mobiles et Internet coupés, robot toujours joignable) : pas de bandeau sur l'écran principal, et les Réglages affichent *Impossible de vérifier les mises à jour* avec **Réessayer**.
- [ ] Un robot jamais réglé garde le canal de son firmware : une bêta affiche le Canal de mise à jour *Bêta*. Passer à *Stable* tient après un redémarrage et une mise à jour ; un robot en bêta sur Stable ne se voit pas proposer une stable plus ancienne.
- [ ] `GET /api/info` contient `"channel"` et `"chipFamily"` (`ESP32`, `ESP32-S3` ou `ESP32-C3`, selon la carte).
- [ ] Avec la connexion MQTT, une fois la page du robot ouverte, `update.catwandconnect_firmware` dans Home Assistant affiche la version installée et la dernière, et une notification de mise à jour, avec un lien vers les nouveautés et sans bouton Installer ; après la mise à jour, il est à jour.

## Home Assistant (MQTT)

Utilise un Home Assistant avec le module Mosquitto et l'intégration MQTT ([configuration](home-assistant.fr.md)).

- [ ] Sur un robot neuf, Réglages → Réseau → Home Assistant est désactivé et le robot n'établit aucune connexion MQTT.
- [ ] L'activer avec l'adresse du broker, l'utilisateur et le mot de passe affiche « Connected » en quelques secondes.
- [ ] Un mauvais mot de passe affiche « refused the user or password » ; une mauvaise adresse affiche « Can't reach the broker ». Le joystick reste fluide pendant que le robot réessaie.
- [ ] L'appareil apparaît dans Home Assistant sous le nom du robot, avec Stow, Horizontal/Vertical position, Horizontal/Vertical maximum speed et Activity.
- [ ] Déplacer la baguette avec le joystick met à jour les positions et Activity dans Home Assistant.
- [ ] Régler une position dans Home Assistant y déplace l'axe en douceur. Une valeur au-delà des limites de course s'arrête à la limite.
- [ ] Appuyer sur Stow dans Home Assistant range le robot, même pendant que quelqu'un utilise le joystick.
- [ ] Changer une vitesse maximale dans Home Assistant se voit dans les Réglages du robot, et une valeur de 5 °/s est ignorée.
- [ ] Débrancher le robot rend ses entités indisponibles dans Home Assistant en une minute environ. Le rebrancher les rend de nouveau disponibles.
- [ ] Renommer le robot : après son redémarrage, Home Assistant affiche le nouveau nom.
- [ ] L'appareil montre aussi un interrupteur Play session et un nombre Session duration (1–60 min) égal à la durée affichée sur l'écran principal.
- [ ] Sur un robot calibré, allumer Play session lance une session pour la durée de session ; Activity affiche `playing`. L'éteindre range la baguette et l'interrupteur reste éteint.
- [ ] Une session lancée depuis l'écran principal allume l'interrupteur dans Home Assistant ; quand le temps est écoulé (essaie 1 min), l'interrupteur s'éteint tout seul. Toucher le joystick ou appuyer sur Ranger l'éteint aussi.
- [ ] Avant la calibration, allumer Play session ne fait rien : l'interrupteur revient sur éteint et le log du robot affiche « [play] Play session refused: not_calibrated, from home_assistant_mqtt ».
- [ ] L'appareil affiche aussi un événement Play session et Sessions today. Démarrer, arrêter et faire refuser des sessions depuis l'écran principal et depuis Home Assistant déclenche chaque fois l'événement avec les bons `event_type`, `source` et `reason` (Outils de développement → États), et jamais un ancien événement quand Home Assistant ou le robot redémarre.
- [ ] Après un redémarrage du robot, Sessions today est inconnu jusqu'à ce qu'on ouvre la page du robot sur un téléphone, puis affiche les sessions lancées aujourd'hui.
- [ ] Changer Session duration dans Home Assistant met à jour la durée sur l'écran principal, et elle est toujours là après un redémarrage. Une valeur de 0 ou 61 est refusée.
- [ ] Pendant une session de jeu, `sensor.catwandconnect_play_session_end` (Play session end) indique quand elle se termine (« dans 9 minutes » pour une session de 10 minutes, comme le décompte de l'écran principal à quelques secondes près) ; après Arrêter ou à la fin du temps, il affiche *Inconnu*. Redémarrer Home Assistant pendant une session fait revenir la même heure de fin.

## Intégration native (découverte et API)

- [ ] Le robot est visible en `_catwandconnect._tcp` pour le zeroconf de Home Assistant : avec l'intégration native installée, il apparaît dans Paramètres → Appareils et services → Découvert ; sans elle, `avahi-browse -rt _catwandconnect._tcp` (Linux) ou `dns-sd -B _catwandconnect._tcp` (macOS) sur le même réseau le liste sur le port 80 avec les enregistrements TXT `id` (l'identifiant d'appareil du rapport de diagnostic), `api=1`, `version` (celle de la page Infos) et `name` (le nom du robot).
- [ ] `GET /api/info` contient `"api":1` et un `"id"` égal à l'identifiant d'appareil du rapport de diagnostic.
- [ ] `curl -X POST -H 'Content-Type: application/json' -d '{"axis":"horizontal","degrees":90}' http://<nom>.local/api/position` y déplace la baguette en douceur et répond `202`. Une valeur au-delà des limites de course s'arrête à la limite ; pendant une session de jeu, elle termine la session sans ranger ; `{"axis":"up","degrees":90}` répond `400`.

## Intégration native (Home Assistant)

Utilise un Home Assistant 2026.10 ou plus récent avec HACS, sur le réseau du robot, et un robot sans la connexion MQTT ([configuration](home-assistant.fr.md#intégration-native)). Vérifie les entités sur la page de l'appareil, dans Paramètres → Appareils et services → CatWandConnect.

- [x] HACS → Dépôts personnalisés accepte `https://github.com/vistorx/ha-catwandconnect` comme Intégration ; CatWandConnect se télécharge et, après un redémarrage, apparaît dans Ajouter une intégration.
- [ ] Le robot apparaît sous Découverts avec son nom. Sans code PIN des réglages, Ajouter puis Valider crée l'appareil sans demander de code PIN : nom, fabricant CatWandConnect, modèle, puce, version du firmware, et un lien Visiter qui ouvre l'interface du robot.
- [x] Une nouvelle découverte du robot (redémarrer Home Assistant ou le robot) ne crée pas de second appareil.
- [x] Ajouter une intégration → CatWandConnect avec l'adresse IP du robot, puis avec son nom en `.local`, l'ajoute (supprime-le entre les deux).
- [x] Avec un code PIN des réglages défini, l'ajout du robot le demande : un mauvais code PIN affiche « Code PIN incorrect » et laisse réessayer ; assez de codes faux affichent l'attente en secondes ; le bon code PIN l'ajoute.
- [x] Un robot avec un firmware plus ancien que 0.2.0-beta.2 est refusé avec « firmware trop ancien », la version minimale et un lien vers la mise à jour.
- [x] L'appareil a 8 entités avec les identifiants par défaut (`sensor.catwandconnect_activity`, `switch.catwandconnect_play_session`, `number.catwandconnect_session_duration`, `button.catwandconnect_stow`, `number.catwandconnect_horizontal_position`, `number.catwandconnect_vertical_position`, `number.catwandconnect_horizontal_maximum_speed`, `number.catwandconnect_vertical_maximum_speed`), nommées en français quand Home Assistant est en français.
- [x] Activité suit le robot en moins d'une seconde : `manual` avec le joystick, `stowing`, `calibrating`, `tuning_speed`, `playing`, puis `idle`.
- [x] Déplacer la baguette avec le joystick met à jour les deux positions en moins d'une seconde. Régler une position dans Home Assistant y déplace l'axe en douceur ; le curseur ne couvre que les limites de course.
- [x] Changer une vitesse maximale dans Home Assistant se voit dans Réglages → Mouvement sur le robot ; un changement dans les Réglages du robot se voit dans Home Assistant après avoir rechargé la page de l'appareil.
- [x] Durée de session est égale à celle de l'écran principal ; la changer dans Home Assistant met à jour l'écran principal, et 0 ou 61 est refusé.
- [ ] Avec une intégration plus récente que la 0.2.0, Fin de la session de jeu (`sensor.catwandconnect_play_session_end`) indique quand la session de jeu en cours se termine, comme le décompte de l'écran principal à quelques secondes près, et ne bouge pas pendant le décompte ; après Arrêter ou à la fin du temps, il affiche *Inconnu*.
- [x] Sur un robot calibré, allumer Session de jeu lance une session pour la durée de session ; l'éteindre range la baguette et l'interrupteur reste éteint. Une session lancée depuis l'écran principal allume l'interrupteur, qui s'éteint tout seul à la fin de la session.
- [x] Avant la calibration, allumer Session de jeu affiche une erreur qui dit que le robot n'est pas calibré, et l'interrupteur reste éteint.
- [x] Ranger dans Home Assistant range le robot, même pendant que quelqu'un utilise le joystick.
- [x] Débrancher le robot rend toutes les entités indisponibles en quelques secondes ; le rebrancher les rend de nouveau disponibles toutes seules, avec les valeurs actuelles.
- [x] Redémarrer Home Assistant avec le robot allumé fait revenir les entités sans rien faire.
- [x] Renommer le robot : après son redémarrage, le nom de l'appareil dans Home Assistant suit.
- [x] Changer le code PIN des réglages du robot : la commande suivante qui en a besoin (une vitesse maximale) échoue et Home Assistant propose Reconfigurer ; saisir le nouveau code PIN refait marcher les commandes.
- [x] Télécharger les diagnostics sur la page de l'appareil : le fichier contient le firmware, les réglages et l'état, avec le nom du Wi-Fi, les adresses IP et MAC, l'adresse du broker et les identifiants MQTT masqués, et aucun code PIN.
- [x] Avec la connexion MQTT activée en plus, un second appareil apparaît avec des identifiants qui finissent par `_2`, comme le guide l'annonce ; désactiver MQTT sur le robot et supprimer l'appareil MQTT en laisse un seul.
- [x] Le tableau de bord tout prêt de l'intégration native, collé tel quel depuis le guide, affiche toutes les cartes sans « Entité non disponible ».
- [ ] Avec l'intégration 0.3.0 ou plus récente et un firmware qui a le journal, l'appareil a un événement Session de jeu et Sessions aujourd'hui. Lancer une session depuis l'écran principal déclenche `started` avec `source` `device_interface` ; éteindre Session de jeu dans Home Assistant déclenche `stopped` avec `source` `home_assistant_native` et `reason` `stop`. Après un redémarrage du robot, Sessions aujourd'hui redevient connu dès que l'intégration se reconnecte, sans ouvrir la page du robot. Avec un firmware plus ancien, aucune des deux n'est créée et les autres marchent.
- [x] La carte joystick (intégration 0.1.1 ou plus récente) est dans le sélecteur de cartes ; maintenir et faire glisser déplace la baguette, lâcher l'arrête ; elle est grisée quand le robot est hors ligne.

## Flasher web et versions publiées

- [ ] La page de la version publiée contient un `-factory.bin` et un `-ota.bin` par puce, et la version de la page Infos correspond au tag.
- [ ] Le flasher web s'ouvre dans Chrome ou Edge sur un ordinateur et affiche la version publiée. Dans Safari ou Firefox, il explique qu'il faut Chrome ou Edge.
- [ ] Avec une carte ESP32, une ESP32-S3 et une ESP32-C3 tour à tour : « Connect and install » détecte la puce et installe. L'effacement lors d'une première installation fonctionne.
- [ ] Après l'installation, « Connect to Wi-Fi » configure le Wi-Fi du robot et « Visit device » ouvre son interface.
- [ ] Installer par-dessus un robot existant sans effacer garde ses réglages.
