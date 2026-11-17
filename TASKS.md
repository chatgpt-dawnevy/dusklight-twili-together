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
  Le partage des défaites couvre dans le code les limaces placées ou générées,
  les Armos, les Moldorms ordinaires et les masques de Xanto combattants.
  Les déplacements et attaques de ces profils restent à traiter. La validation
  des générations de limaces et des interrupteurs reste prévue.
  Pour le Grand Baba, le partage des PV de la tête et de la défaite
  finale de la fleur est implémenté. Le partage immédiat de la mort
  de la tête, les attaques et l’obtention de la clé restent à vérifier.
  La division du grand Freezard et l’identité de ses quatre petits
  sont implémentées. Les impacts sur le parent et le comportement
  conjoint des quatre acteurs restent à vérifier.
  La défaite du Hebi Baba et la consommation du Bomb Insect sont
  partagées dans le code. Les récompenses et les interactions avec les
  bombes restent à vérifier à deux. Le Cavalier fantôme partage les
  dégâts et la défaite au combat. La fin de son délai d’apparition
  reste locale. Son cycle complet reste à vérifier à deux. Les deux
  variantes du Baba du Crépuscule partagent leur défaite et conservent
  les effets de disparition. Le butin et les attaques restent à vérifier.
- [ ] Ennemis à un PV : tester les morts natives des rats, Leevers, Bombskits, Keese d'ombre et autres
  variantes. Vérifier les générations lors d'une mort suivie d'une nouvelle apparition.
  Les bébés Gohma placés à 1 PV partagent leurs défaites à l’épée et
  à la statue. Leurs interrupteurs ont un test ciblé à deux clients.
- [ ] Crépuscule : vérifier tous les insectes, y compris les variantes de la rivière, les larmes uniques
  et la progression après collecte simultanée, changement de zone et reconnexion.
  Les quatre placements de rivière, leurs larmes et l’interrupteur 0x68
  passent le scénario Linux à deux. Le vol complet reste à jouer.
- [ ] Boss et mini-boss : inventorier les profils et acteurs secondaires. Tester chaque phase, projectile,
  vulnérabilité, scène et récompense. Vérifier une seule récompense et une sortie utilisable à deux.
  Argorok passe la fin, le miroir, le cœur et la sortie dans les
  deux directions du scénario Linux. Les deux ruptures d’armure
  déclenchées par l’un ou l’autre joueur mènent les deux clients à
  la phase vulnérable dans les scénarios. Les deux directions de mise à
  terre de Fyrus et d’étourdissement du crapaud Mojo passent également.
  La défaite du crapaud Mojo, un seul coffre de grappin et la porte de
  sortie sont vérifiés dans les deux directions. La défaite de
  Blizzeta est testée avec les deux joueurs en seconde forme. Ses six
  ruptures d’armure et l’arrivée en seconde forme sont vérifiées
  lorsque les joueurs alternent les coups dans les deux ordres.
  Les deux coups non létaux de la seconde forme et la destruction des
  dix glaçons passent également dans les deux directions. Les attaques,
  les positions et le combat complet restent à vérifier. Le partage des dégâts
  et de la défaite du chevalier au boulet est implémenté dans sa salle de combat.
  Le boulet, l’obtention de l’objet et la sortie attendent leur validation.
- [ ] Objets et progression : vérifier les acquisitions et dépenses dans les deux modes de stock, les
  achats, bouteilles, échanges, clés, checks randomisés et exploration séparée suivie d'un rattrapage.
  Deux clients Linux ont réconcilié objets, événement et interrupteurs
  persistants après exploration séparée et déconnexion.
- [ ] Robustesse : exécuter les suites existantes et les régressions avec transferts, retransmissions,
  désordre, 50/100/200 ms, latence variable, pertes, mort de Link et changements de room/layer.
  Le maintien WebSocket après 35 secondes au repos est vérifié sur Linux.
- [ ] Réception : finir les campagnes vanilla et randomizer à deux. Tester Linux/Linux, Windows/Windows
  et Windows/Linux, dont une session WAN. Utiliser des sauvegardes isolées et aucun écran utilisateur.
- [ ] Livraison : publier sur le fork deux fichiers .dusk x86-64 distincts du même commit, la compatibilité
  Dusklight, l'installation, la configuration du relais Internet, les preuves et les limites.
  Vérifier les conditions de redistribution et l'absence de contenu protégé dans les paquets.
- [x] Conserver les PR et utiliser uniquement **Squash and merge** pour leur fusion.
  Le workflow Windows/Linux et relais est manuel sur main.
