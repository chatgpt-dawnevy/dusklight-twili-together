# Twili-Together — avancement

## 2026-11-16

- Le fork public est `chatgpt-dawnevy/dusklight-twili-together`. Base amont `0c61df4`.
- Les PR #1 à #3 ont intégré la base de développement, l'audit et le séquencement reçu.
  Les PR #4 (reprise après déconnexion) et #5 (déduplication au relais) ont réussi les
  compilations Windows/Linux et les tests CI du relais, puis ont été intégrées à `main`
  (`9fb12a0`). Reconnexion à deux vérifiée
  localement sous Linux, sans fenêtre sur le bureau.
- Les derniers essais de boss ciblés sont documentés dans l'audit. Le scénario de récompenses
  de Blizzeta a échoué et son correctif expérimental a été retiré.
- Les tests Linux utilisent le SDK épinglé à `a6dcc2e726d9c94b033cd08634dfaebf62ba4468`,
  le moteur de test local et Xvfb. Les scénarios automatisés ne remplacent pas les sessions
  manuelles Windows/Linux ni la campagne complète.
- Diagnostic Leever : les enfants natifs d'un nid E_rb n'ont pas de placement direct
  et n'utilisent pas la liste E_Make. Ils étaient exclus du suivi faute de clé.
  Ajout d'une clé fondée sur le nid et l'ordre de ses demandes de création. Les numéros
  continuent au fil des vagues. Le rechargement du nid exige encore une génération de zone.
  Le scénario `enemy-leever-shadow-keese-native` passe chez A et B (commit
  `36b850f`, PR #6). A déclenche la réaction létale du Leever et B celle du Keese.
  Les deux suppressions natives, les interrupteurs 26 et 27 et les compteurs de
  réseau sont vérifiés. Le premier essai exigeait à tort un PV courant de 1 chez
  le Leever ; le jeu met ce champ à zéro même quand il est vivant. Le test corrigé
  vérifie sa capacité native d'un PV. Preuves dans
  `tests-coop/enemy-results/20261116-143849/enemy-leever-shadow-keese-native/`.
- Prochaine action : vérifier deux enfants d'un même nid et une seconde vague,
  puis ajouter l'incarnation de zone aux clés réseau pour refuser les morts tardives
  après un rechargement. Tester ensuite l'attribution d'un coup simultané à l'autorité.
- Le scénario `enemy-leever-sibling-identity` passe chez A et B. Les deux enfants
  portent les identifiants 0xC000 et 0xC001. A élimine le second, les deux copies
  gardent le premier vivant et l'interrupteur fermé. B élimine ensuite le premier
  et les deux sauvegardes reçoivent l'interrupteur. Prochain test : génération
  suivante d'un nid sans interrupteur, puis rechargement complet de la salle.
  Preuves dans `tests-coop/enemy-results/20261116-144039/enemy-leever-sibling-identity/`.
- PR #7 fusionnée dans `main` (`15fa1bd`). Ancien workflow de builds automatiques
  retiré. La validation Windows/Linux et relais est déclenchée manuellement sur main.
- `enemy-leever-next-brood` passe sur les deux clients Linux isolés. La première
  vague utilise 0xC000 et la suivante 0xC001. A déclenche la première défaite,
  B la seconde. Preuves dans `tests-coop/enemy-results/20261116-144809/`.
- PR #6 intégrée à `main` (`9c6dd63`). Le premier test de mort différée a
  révélé qu'une suppression simple était classée comme une défaite consommée.
  Le correctif lie l'événement différé à l'incarnation locale qu'il a trouvée.
  Le scénario `enemy-deferred-defeat-replaced-actor` passe chez A et B sous
  Linux : le remplacement au même placement survit, l'événement expire et
  aucune défaite n'est appliquée. Preuves dans
  `tests-coop/enemy-results/20261116-145153/enemy-deferred-defeat-replaced-actor/`.
- Prochaine action : définir une génération partagée de zone au relais et
  la vérifier à la réception des coups et des morts. La liaison locale ne
  protège pas contre un paquet reçu pour la première fois après rechargement.
- PR #8 fusionnée dans `main` (`e8360a7`) après les scénarios de remplacement
  et rat/Bombskit/insecte d'ombre, réussis par les deux clients Linux.
- Branche `feature/enemy-zone-delivery` : le relais marque les paquets par
  génération annoncée du destinataire. Les files locales expirent au déchargement
  de la table de sauvegarde. Les tests ciblés du relais et la compilation Linux
  ont réussi, ainsi que les 127 tests du relais. Le scénario avec deux clients
  Linux a vérifié qu'une défaite injectée dans le gestionnaire avec une ancienne
  génération laisse le Bokoblin vivant, puis qu'avec la génération courante la
  même défaite s'applique. Régression réseau Bokoblin réussie sur les deux clients.
  Preuves : `tests-coop/enemy-results/20261116-145714/`.
  Cette étape ne traite pas un événement ancien reçu tardivement par le relais.
- Prochaine action : associer le producteur de l'événement à une génération du
  combat attribuée par le relais, puis tester un coup ancien reçu après le
  rechargement. Vérifier la reprise des PV avant d'étendre aux boss.
- PR #9 fusionnée par **Squash and merge** après réussite des 127 tests du relais
  et des deux scénarios Linux de réception et de combat Bokoblin.
- La branche suivante marque la génération côté émetteur. Le test du relais
  vérifie qu'un paquet lié à son ancienne génération est refusé après annonce
  du chargement suivant. Compilation Linux et 127 tests du relais réussis.
  Les deux clients Linux isolés ont réussi `enemy-leever-next-brood` et
  `enemy-bokoblin-combat`. Preuves locales sous `tests-coop/enemy-results/`,
  exécutions `20261116-150748` et `20261116-150927`.
- Prochaine action : mettre au point un point de reprise des PV du Bokoblin
  ordonné avec les dégâts fiables et tester son application après une
  reconnexion sans doubler un coup déjà reçu.
- PR #10 fusionnée par squash (`82fd209`).
- Branche `feature/bokoblin-health-recovery` : les instantanés d'autorité
  réconcilient les PV non létaux d'un Bokoblin. La compilation Linux,
  les 127 tests du relais, le test d'isolation de l'affichage et le test
  de routage du checkpoint ont passé. Première exécution : le signal de
  rendez-vous est parti avant l'arrivée de B. Après correction du scénario,
  les deux clients Linux sont passés de 40 à 30 PV après reconnexion de A,
  puis de 30 à 22 PV avec un seul message de dégâts appliqué chez A.
  Le scénario de transfert d'autorité après reconnexion passe également.
  Preuves : `tests-coop/enemy-results/20261116-151507/`.
- Prochaine action : traiter l'insecte de rivière avec les acteurs
  réellement placés, en vérifiant la cible détenue par le Kargorok et
  l'unicité des larmes. Le compteur et les récompenses restent à vérifier
  sur une séquence complète.
- PR #11 fusionnée par squash (`2d68109`). Reconnexion et transfert de
  l'autorité Bokoblin vérifiés sur deux clients Linux.
- Inventaire de l'ISO local : 49 placements `E_ym` dans les archives de
  salles et de scènes inspectées, dont quatre de type 7, tous à
  `F_SP112`, salle 1, couche 13. Les TBOX correspondants sont
  0x36, 0x3A, 0x39 et 0x3D. Leurs interrupteurs alimentent celui de
  progression 0x68. Le profil `E_cr` est le Bombskit dans le SDK ;
  les 27 placements relevés ont un octet bas 0xFF. La recherche
  d'alias « crab » dans les acteurs placés n'a rien donné.
- Branche `feature/river-tear-sync` : libérer la cible du Kargorok lors
  de la défaite distante et tester l'activation des interrupteurs, la
  disparition des acteurs et la collecte des larmes. La compile Linux et
  les 127 tests du relais passent. Le premier scénario avec transition
  de capture injectée n'a pas déclenché la défaite locale ; relance en
  appelant la fin de capture native sur l'acteur placé.
- Le scénario `enemy-kargorok-river-insects` passe chez les deux clients
  Linux (Xvfb privé). Deux captures natives déclenchées par client, deux
  défaites distantes sans écho, cible du Kargorok libérée, quatre larmes
  collectées une fois chacune et interrupteur 0x68 déclenché dans les
  deux parties. Le scénario stationnaire termine les événements locaux
  et exécute les routines des acteurs suspendus. Le vol complet et la
  concurrence sur la même larme pendant le vol restent à vérifier.
  Preuves : `tests-coop/enemy-results/20261116-152628/`. Le scénario de
  régression rat/Bombskit/insecte d'ombre passe également
  (`20261116-152100/enemy-one-hit-and-shadow-insect/`).
- Prochaine action : inventorier les profils classiques non couverts,
  puis traiter les Chuchus en tenant compte des fusions et des gelées.
- PR #12 fusionnée par squash (`d935939`).
- Relevé croisé des profils `fopAc_ENEMY_e` du SDK et des placements de l’ISO :
  87 profils ennemis placés. Sur la base de la PR #12, 41 ne figurent dans
  aucune des listes de partage des dégâts ou des défaites. Le relevé des
  placements ne couvre pas les acteurs créés uniquement à l’exécution.
- Branche `feature/chuchu-defeat` : un petit Chuchu placé qui passe à
  l’état de gelée déclenche chez le coéquipier la même phase native.
  Les copies agrandies ou ayant changé de couleur ne sont pas remplacées
  par une gelée distante. Les grandes formes, la fusion, la division et
  les gelées en bouteille restent à synchroniser. Les deux clients
  Linux ont réussi `enemy-chuchu-jelly` et les 127 tests du relais
  passent. Le scénario de copie agrandie passe sur deux clients
  (`tests-coop/enemy-results/20261116-201054/enemy-chuchu-merged-copy/`).
- Prochaine action : numéroter les messages de dégâts fiables au relais,
  puis vérifier deux coups reçus en concurrence et les retransmissions.
- PR #13 fusionnée par squash (`d712ba6`). La relance des deux scénarios
  Chuchu passe sur deux clients Linux avec le même paquet
  (`tests-coop/enemy-results/20261116-201054/`).
- Branche `feature/enemy-damage-sequence` : le relais écarte les lots de
  dégâts retransmis par une même connexion. La compilation Linux et
  les 128 tests du relais passent. Avec 0 à 120 ms de latence ajoutée,
  chaque client applique exactement un coup distant : 40 - 7 - 11 = 22 PV
  des deux côtés. `enemy-damage-share` et `enemy-bokoblin-health-reconnect`
  passent aussi sur les deux clients. Preuves sous
  `tests-coop/enemy-results/20261116-201404/`.
- Prochaine action : déclencher et vérifier la fin native d’Argorok,
  avec son acteur de scène, le conteneur et la sortie.
- PR #14 fusionnée par squash (`7e91335`).
- Branche `feature/argorok-native-defeat` : le coup final repéré sur
  Argorok déclenche chez le partenaire la routine de frappe sur son
  point faible. Le jeu crée l’acteur de fin B_DRE et applique les
  drapeaux de victoire. Le premier essai a rencontré une division par
  zéro dans le vol natif de la sauvegarde de test stationnaire. La
  sauvegarde instrumentée initialise son paramètre de virage et place
  le boss en deuxième phase. Le scénario A vainqueur a confirmé sous
  Linux un conteneur de cœur et une sortie pour chaque client
  (`tests-coop/boss-results/20261116-202036/`). Les deux directions,
  l’éclat de miroir, le cœur partagé et l’utilisation de la sortie
  sont en cours de validation sans affichage visible.
- Validation étendue avec A vainqueur réussie sur deux clients Linux.
  Chaque partie a reçu l’éclat de miroir et un seul cœur supplémentaire
  après un seul ramassage. Les deux joueurs sont sortis vers
  `D_MN07`, salle 0, entrée 4, avec les drapeaux de victoire conservés.
  Les 128 tests du relais et le test de l’affichage isolé passent.
  Le scénario inverse est également réussi.
  Preuves : `tests-coop/boss-results/20261116-202304/`.
- Prochaine action : synchroniser les deux ruptures d’armure d’Argorok
  avec les routines natives et vérifier que le coéquipier peut atteindre
  la phase vulnérable sans répéter la même manœuvre.
- PR #15 fusionnée par squash (`051e8e9`).
- Branche `feature/argorok-armor-phases`. Le premier essai a validé la
  première rupture sur les deux clients. B a créé son acteur B_DRE
  pour la seconde rupture, puis le test a expiré pendant la scène.
  La relance annonce la seconde rupture dès son entrée native
  confirmée, pour éviter que B attende la fin de la scène chez A.
  Le scénario actuel fait démarrer les ruptures chez l’autorité.
  Il reste à vérifier leur déclenchement par le coéquipier.
- Les deux ruptures d’armure passent sur deux clients Linux isolés
  avec l’autorité comme initiateur. Les deux scènes B_DRE se
  déroulent en parallèle. B atteint la phase vulnérable environ
  2,4 secondes après A. Compilation Linux, 128 tests du relais et
  vérification ciblée du routage de l’armure réussies.
  Preuves dans `tests-coop/boss-results/20261116-203353/`.
- Prochaine action : permettre au coéquipier qui ne détient pas
  l’autorité de lui transmettre sa rupture d’armure et valider ce
  sens avec les deux instances Linux isolées.
- PR #16 fusionnée par squash (`31cb0dd`).
- Branche `feature/argorok-peer-phase`. Le coéquipier annonce sa
  rupture à l’autorité de l’arène, qui joue la scène native et
  publie sa propre phase. Les deux clients ont passé le scénario
  où B, sans autorité, déclenche les deux ruptures. La compilation
  Linux et les 129 tests du relais passent. Preuves dans
  `tests-coop/boss-results/20261116-203813/`. La régression dans
  l’autre sens passe également sur les deux clients.
- Les tests de stock en zones différentes passent dans les deux modes
  (`tests-coop/stock-results/20261116-203851/`). Le portail, deux clés,
  deux âmes et une bouteille sont partagés dans le scénario ciblé du
  randomizer (`tests-coop/results/20261116-203937/`). Ce scénario
  utilise le mode randomizer sans partie complète avec seed.
- Prochaine action : donner à chaque joueur la possibilité de mettre
  Fyrus à terre, puis vérifier le passage aux états natifs de Fyrus
  dans les deux directions.
- PR #17 fusionnée par squash (`59f7bb1`).
- Avec le randomizer chargé, deux clients partageant le même seed de
  test échangent leurs objets. Deux seeds de test différents restent
  isolés. Les deux scénarios utilisent des fichiers de seed et un
  résolveur simulé qui fournit les placements. Les résultats sont dans
  `tests-coop/results/20261116-204051/`. Il reste à terminer une
  campagne avec les placements générés et chargés par le randomizer.
- Branche `feature/fyrus-peer-knockdowns`. Le coéquipier peut transmettre
  son compteur de mises à terre natives à l’autorité de Fyrus. Le relais
  vérifie le profil, l’arène, la salle et la valeur du compteur. La
  compilation Linux passe. Les tests de scène et les régressions du
  relais sont en cours.
- Fyrus atteint la phase vulnérable chez les deux clients lorsque le
  joueur qui détient l’autorité porte le coup, et lorsque son partenaire
  le porte. Les deux scénarios utilisent l’entrée native de mise à terre.
  Les 129 tests du relais et la compilation Linux passent.
  La régression de l’étourdissement du crapaud Mojo passe également.
  Preuves dans `tests-coop/boss-results/20261116-204354/`.
- Prochaine action : transmettre un étourdissement du crapaud Mojo une
  seule fois par entrée native, quel que soit le joueur qui le déclenche.
- PR #18 fusionnée par squash (`c762c11`).
- La récupération d’un objet et d’interrupteurs après une période
  hors ligne passe, ainsi que la récupération d’une clé sans rejouer
  les clés déjà reçues. Preuves sous
  `tests-coop/results/20261116-204619/`.
- Branche `feature/toad-peer-stun`. Le coéquipier peut annoncer le
  début de la réaction à une bombe. Le relais la transmet uniquement
  à l’autorité. Les confirmations reçues durant cette réaction ne
  relancent pas l’animation. Compilation Linux et 129 tests du relais
  réussis. Les deux scénarios à deux clients vérifient le début et
  la fin de l’étourdissement, puis l’absence de reprise. Preuves
  dans `tests-coop/boss-results/20261116-205034/`.
- Prochaine action : vérifier la défaite du crapaud Mojo, son coffre
  de grappin et la sortie de l’arène dans les deux directions.
- PR #19 fusionnée par squash (`8c5eb65`).
- Branche `test/toad-native-reward`. Le scénario A vainqueur confirme
  la défaite native du crapaud Mojo sur les deux clients, la disparition
  du boss et le drapeau de mini-boss. A ouvre le coffre natif une seule
  fois. B obtient le grappin et ne peut pas rouvrir le coffre. Les deux
  clients empruntent la porte vers `D_MN01`, salle 9.
  Le scénario inverse passe également. Compilation Linux et
  129 tests du relais réussis. Preuves sous
  `tests-coop/boss-results/20261116-205524/`.
- Le relevé de l’ISO identifie 79 bébés Gohma placés de type 2
  dans quatre salles du Temple du Temps. Les formes de boss et les
  enfants créés à l’exécution demandent leurs propres règles.
- Prochaine action : détecter les morts natives des bébés Gohma
  placés à 1 PV et préserver les différences entre l’épée et la statue.
- PR #20 fusionnée par squash (`4e631ce`).
- Branche `feature/baby-gohma-defeat`. Les deux clients Linux exécutent
  une défaite native chacun à partir de l’entrée de réaction. La défaite
  à l’épée active l’interrupteur des deux côtés. Le coup de statue laisse
  l’autre interrupteur intact. Une simple suppression reste locale.
  Le scénario désactive la synchronisation du monde pour isoler le
  paquet de défaite. Compilation Linux et 129 tests du relais réussis.
  Le scénario rat/Bombskit/insecte d’ombre passe également. Preuves dans
  `tests-coop/enemy-results/20261116-210006/`.
- Prochaine action : reproduire puis corriger les pertes WebSocket
  observées pendant l’attente d’un autre joueur, avant d’étendre les
  tests de progression séparée.
- PR #21 fusionnée par squash (`e3c29c4`).
- Branche `fix/websocket-idle-heartbeat`. Le test de référence a reproduit
  la coupure après environ 15 s. Le premier client a repris la même partie
  avec un nouvel identifiant. Le nouveau message de maintien WebSocket
  passe les 130 tests du relais, dont le maintien d’une connexion qui
  ne renvoie pas de Pong et l’expiration d’un client devenu silencieux.
  Compilation Linux réussie. Tests en jeu en cours sur l’attente,
  la reconnexion après arrêt du relais et TCP.
- Le client WebSocket corrigé est resté connecté pendant les 35 s
  d’attente avec son identifiant initial. Preuves dans
  `tests-coop/results/20261116-220810/ws-idle-heartbeat/`.
- Après un arrêt et redémarrage du relais, les deux clients WebSocket
  se reconnectent et se retrouvent. Les deux clients TCP passent
  également le scénario de présence.
- Prochaine action — vérifier que deux joueurs peuvent avancer dans
  des donjons différents, évoluer pendant une déconnexion, puis
  réunir leurs objets et interrupteurs en se retrouvant.
- PR #22 fusionnée par squash (`629cb2a`).
- Branche `test/split-dungeon-progress`. Deux clients Linux progressent
  au Temple Sylvestre et dans les Mines Goron. Après la déconnexion
  d'un joueur, les deux récupèrent ses objets et l'événement déclenché
  par son partenaire. En échangeant de donjon, chacun retrouve
  l'interrupteur persistant de l'autre, puis tous deux se rejoignent
  avec les deux objets. Les interrupteurs DAN utilisés lors du premier
  essai expirent à la sortie. Le scénario vérifie les interrupteurs
  persistants et l'isolation entre les deux tables de sauvegarde.
  Preuves dans `tests-coop/results/20261116-221420/split-dungeon-progress/`.
- Prochaine action — reprendre la validation de la défaite de Blizzeta.
- PR #23 fusionnée par squash (`e0fcf8d`). GitHub n’a lancé aucun build
  automatique sur la PR.
- Branche `feature/blizzeta-native-clear`. La défaite distante attend
  que la seconde forme soit prête avant de lancer la scène native.
  L’ancien test forçait la fin dans la première forme et le boss
  ne rencontrait jamais le mur requis par la scène. Les nouveaux
  scénarios lancent d’abord la transition native sur les deux clients.
  La compilation Linux et les 130 tests du relais passent.
- Un délai intermédiaire de 120 s interrompait le scénario après le
  dialogue de Yeta. Le délai révisé a permis de vérifier la scène
  BOSSCLEAR05 unique, l’éclat de miroir, le cœur en un exemplaire,
  son partage et la sortie vers la montagne, quel que soit le joueur
  qui porte le coup final.
  Preuves dans `tests-coop/boss-results/20261116-222551/`.
- Prochaine action — synchroniser les ruptures d’armure de Blizzeta
  et l’entrée en seconde forme entre les deux joueurs.
- PR #24 fusionnée par squash (`0a7d4b2`).
- Branche `feature/blizzeta-armor-phase`. Le relais accepte les
  propositions de Blizzeta de 1 à 7 exclusivement dans son arène.
  L’autorité et son partenaire appliquent chaque réaction d’armure
  dans l’ordre, puis jouent la scène native de la seconde forme.
  La compilation Linux et les 130 tests du relais passent. Les deux
  clients Linux ont confirmé les six ruptures et la seconde forme
  dans les deux ordres, avec un seul joueur déclenchant chaque coup.
  Preuves dans `tests-coop/boss-results/20261116-223811/`.
  Les trois régressions Argorok, Fyrus et crapaud Mojo passent sous
  Linux avec le même paquet (`tests-coop/boss-results/20261116-224114/`).
- Prochaine action — partager les deux réactions non létales de la
  seconde forme et vérifier l’état des glaçons après chaque coup.
- PR #25 fusionnée par squash (`71f6dc0`).
- Branche `feature/blizzeta-second-form-damage`. La compilation Linux
  et les 130 tests du relais passent. Le premier essai à deux a confirmé
  la réaction native et l’éclatement des dix glaçons après le premier
  coup de B. Le second coup n’a pas atteint sa fenêtre dans le scénario.
  La relance attend la vulnérabilité native et précise l’action,
  le mode, la hauteur et le masque de glaçons en cas d’échec.
  Preuves dans `tests-coop/boss-results/20261116-224932/` et
  `tests-coop/boss-results/20261116-225439/`.
- Le relevé local actualisé de l’ISO trouve 87 profils ennemis placés.
  Sur le commit de la PR #25, 38 n’ont aucune entrée explicite de
  dégâts ou de défaite dans les listes du mod. Le SDK définit aussi
  21 profils ennemis sans placement direct dans ces archives.
- Les essais mixtes avec les paquets du commit `9fb12a0` et le moteur
  Windows v2.0.3 sous Wine ont échoué avant la connexion du client
  Windows. WineD3D plante, DXVK refuse la création du contexte D3D11
  et le rendu Vulkan ne dépasse pas l’initialisation dans Xvfb.
  Le test en compositeur virtuel échoue à créer la fenêtre Windows.
  Un essai sur un hôte Windows natif reste nécessaire.
- Le randomizer 1.0.6 a généré puis activé le seed réel
  `Lanayru Bokoblin Zelda` dans une nouvelle partie isolée.
  Les journaux et le seed se trouvent dans
  `tests-coop/native-seed-keyboard-work/`.
- Prochaine action — permettre au lanceur de conserver une sauvegarde
  du randomizer, la charger par le menu natif, puis vérifier à deux
  l’ouverture, l’objet accordé et la déduplication d’un coffre du seed.

## 2026-11-17

- PR #26 fusionnée par squash (`e1b9e03`) après la validation de Blizzeta.
- La PR #27 contient le scénario préparé pour un coffre sur un seed
  réel, avec chargement natif de la sauvegarde.
- Branche `feature/torch-slug-native-death`. Les placements au sol,
  au mur et au plafond utilisent leur action de défaite native à la
  réception du message. Leurs interrupteurs restent pilotés par cette
  action. Seule la copie source effectue le tirage du butin pour une
  défaite reçue. Les générateurs et leurs enfants sont encore hors de
  ce correctif. Aucun test exécuté pendant cette reprise.
- Prochaine action — vérifier la mort, son interrupteur et son butin
  sur deux clients pour chaque orientation, puis traiter les enfants
  du générateur et les attaques.
