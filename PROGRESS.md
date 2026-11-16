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
- Le même seed du randomizer entraîne un partage sur les deux clients.
  Deux seeds différents restent isolés. Les deux scénarios automatisés
  ont réussi dans `tests-coop/results/20261116-204051/`. Ils ne couvrent
  pas une campagne complète.
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
