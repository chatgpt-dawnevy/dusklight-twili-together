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
