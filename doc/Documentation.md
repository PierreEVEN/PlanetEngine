# Objectif

Au cours de mes précédents projets, j'ai pu traiter la génération procédurale de terrain à de nombreuses reprises sous différents moteurs et architectures :

UE4 ![UE4](https://media.discordapp.net/attachments/360795179076288512/460104259627646986/unknown.png?width=312&height=175)
Unity ![Unity](150693106-8bfea4fe-9458-4c35-ab8a-4935b0b7c076.png)

- Modélisation d'une planète, et d'un système solaire, du sol jusqu'à l'espace, avec possibilité de naviguer entre différents astres sans transitions.
- Précision du maillage de terrain au sol de 1m par vertex.

# Difficultées techniques prévisibles

## Précisions en virgule flottante.

Au dessus de quelques km de distance, l'affichage d'une scène pose quelques problème de précision dus au fonctionnement des nombres à virgule flottante, et commence à générer des artéfacts de rendu comme le montre cette [animation](https://www.shadertoy.com/view/4tVyDK).
En pratique, au bout d'à peine 2km de l'origine le phénomène commence à être perceptible, et devient particulièrement problèmatique dès les premières dizaines de km. C'est largement insuffisant pour un système solaire entier même réduit.

La solution la plus simple est d'utiliser des doubles sur 64 bits (fp64) au lieu des float sur 32 bits (fp32). Grace à ça, les problèmes de précision commencent à apparaitre bien plus loins et permettent sans trop de problèmes d'implémenter un système solaire presque complet. (pour les dernières planètes du système solaire, c'est encore un peu léger mais ce sera suffisant dans un premier temps.)

> Pour ce genre de problèmes la solution idéale serait d'utiliser un système de coordonées utilisant des réels à virgule fixe (avec des int). Comme je ne voulais pas trop aller dans l'inconnu, j'ai préféré rester sur de la virgule flottante classique. (ce sera à essayer pour un futur projet)

## Doubles sur GPU

Si les CPU sont très bon pour faire du calcul en fp64 (un peu moins qu'en fp32), les GPU eux sont très mauvais pour ça, voir incapables de le faire pour certaines opérations. Les données devront donc systématiquement rester en fp32 sur GPU.

## Coordonées de texture 2D

Texturer une sphère proprement est très compliqué. Il n'est pas possible de plaquer texture 2D sur une sphère sans avoir de déformations ou d'irrégularités de la densité de pixels à la surface. Il faudra trouver des méthodes pour contrer ce phénomène et faire des compromis.

# Implémentation

## Caméra et système de coordonées

Pour contrer les problèmes de précision, la scène sera centrée autour de la caméra avant d'être transmise au GPU afin que la précision soit maximale autour du point de vue. Il faudra donc additionner la translation de la caméra à la matrice "model" de tous les éléments de la scène au lieu de la traiter dans la matrice "vue".

Le calcul de la profondeur dans le depth-buffer sera réalisé en virgule flottante afin d'avoir une distance d'affichage infinie. Pour éviter les problèmes de précision on l'inversera pour avoir les plus petites valeurs au premier plan. [Reversed Depth Buffer](https://www.danielecarbone.com/reverse-depth-buffer-in-opengl/)

## Planetes