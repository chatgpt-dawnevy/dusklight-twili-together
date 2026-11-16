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
