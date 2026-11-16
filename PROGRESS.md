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
- Travail en cours : ajouter la preuve de mort native des Leevers et Keese d'ombre.
  Ensuite traiter les incarnations des spawners et le rattrapage des dégâts autoritatifs.
- Diagnostic Leever : les enfants natifs d'un nid E_rb n'ont pas de placement direct
  et n'utilisent pas la liste E_Make. Ils étaient exclus du suivi faute de clé.
  Ajout d'une clé fondée sur le nid et l'ordre de ses demandes de création. Les numéros
  continuent au fil des vagues. Le rechargement du nid exige encore une génération de zone.
  La compilation et le scénario des morts natives sont en cours de validation.
