# Twili-Together — tâches de livraison

Références : [audit](docs/audit.md), [critères de réception](docs/cooperative-acceptance.md),
[protocole](docs/protocol.md), [rapport de tests](docs/test-report.md).
Cocher un critère seulement après vérification de son périmètre complet.

- [x] Confirmer le fork public, la base amont et le SDK épinglé. Conserver les correctifs existants et l'audit.
- [ ] Autorité des combats : définir les générations de zone et de spawn, les baux ou transferts d'autorité,
  les checkpoints de reprise, les instantanés versionnés et la réconciliation après déconnexion.
  Le séquencement des snapshots, la reprise d'une attaque et le rejet d'une ancienne
  génération de destinataire ou d’émetteur ont des tests ciblés.
- [ ] Dégâts et vie : faire valider les coups par l'autorité, appliquer une seule fois les coups simultanés,
  réconcilier les PV, refuser les coups après une mort et préserver les réactions natives.
  La reprise non létale des PV du Bokoblin après reconnexion a un scénario
  validé sur deux clients Linux. Les dégâts simultanés restent à traiter.
- [ ] Ennemis classiques : inventorier placements, variantes et spawners. Synchroniser apparition,
  position et orientation avec interpolation, cibles, attaques, collision, réactions et défaites.
- [ ] Ennemis à un PV : tester les morts natives des rats, Leevers, Bombskits, Keese d'ombre et autres
  variantes. Vérifier les générations lors d'une mort suivie d'une nouvelle apparition.
- [ ] Crépuscule : vérifier tous les insectes, y compris les variantes de la rivière, les larmes uniques
  et la progression après collecte simultanée, changement de zone et reconnexion.
- [ ] Boss et mini-boss : inventorier les profils et acteurs secondaires. Tester chaque phase, projectile,
  vulnérabilité, scène et récompense. Vérifier une seule récompense et une sortie utilisable à deux.
- [ ] Objets et progression : vérifier les acquisitions et dépenses dans les deux modes de stock, les
  achats, bouteilles, échanges, clés, checks randomisés et exploration séparée suivie d'un rattrapage.
- [ ] Robustesse : exécuter les suites existantes et les régressions avec transferts, retransmissions,
  désordre, 50/100/200 ms, latence variable, pertes, mort de Link et changements de room/layer.
- [ ] Réception : finir les campagnes vanilla et randomizer à deux. Tester Linux/Linux, Windows/Windows
  et Windows/Linux, dont une session WAN. Utiliser des sauvegardes isolées et aucun écran utilisateur.
- [ ] Livraison : publier sur le fork deux fichiers .dusk x86-64 distincts du même commit, la compatibilité
  Dusklight, l'installation, la configuration du relais Internet, les preuves et les limites.
  Vérifier les conditions de redistribution et l'absence de contenu protégé dans les paquets.
- [x] Conserver les PR et utiliser uniquement **Squash and merge** pour leur fusion.
  Le workflow Windows/Linux et relais est manuel sur main.
