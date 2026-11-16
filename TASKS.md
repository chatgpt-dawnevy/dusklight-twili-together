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
  validé sur deux clients Linux. Deux coups proches dans le temps ont
  convergé à 22 PV sous latence variable, et le relais rejette les
  lots retransmis. Il reste à faire valider les coups par l’autorité.
- [ ] Ennemis classiques : inventorier placements, variantes et spawners. Synchroniser apparition,
  position et orientation avec interpolation, cibles, attaques, collision, réactions et défaites.
  Le relevé des profils placés est documenté dans PROGRESS.md. Le passage
  à l’état de gelée des petits Chuchus a été testé sur deux clients Linux.
- [ ] Ennemis à un PV : tester les morts natives des rats, Leevers, Bombskits, Keese d'ombre et autres
  variantes. Vérifier les générations lors d'une mort suivie d'une nouvelle apparition.
- [ ] Crépuscule : vérifier tous les insectes, y compris les variantes de la rivière, les larmes uniques
  et la progression après collecte simultanée, changement de zone et reconnexion.
  Les quatre placements de rivière, leurs larmes et l’interrupteur 0x68
  passent le scénario Linux à deux. Le vol complet reste à jouer.
- [ ] Boss et mini-boss : inventorier les profils et acteurs secondaires. Tester chaque phase, projectile,
  vulnérabilité, scène et récompense. Vérifier une seule récompense et une sortie utilisable à deux.
  Argorok passe la fin, le miroir, le cœur et la sortie dans les
  deux directions du scénario Linux. Les deux ruptures d’armure
  déclenchées par l’un ou l’autre joueur mènent les deux clients à
  la phase vulnérable dans les scénarios. Le combat complet reste à jouer.
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
