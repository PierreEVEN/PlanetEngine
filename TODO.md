## Todo

- Ajouter l'IBL des reflections au PBR : Finir les reflections avant.
- Implementer un hierarchical Z-buffer pour le SSR : Faire le downsampling en compute shader.
- Passer le downsampling des framebuffer pour le bloom sur compute shader.
- Faire du Frustum culling : Pas urgent car ne sera que très peu efficace en pratique tant qu'il n'y aura pas plus de géometrie à la surface des planètes.
- Decouper le mesh des planetes en petites grilles fixes plutôt qu'un grand carré qui tourne sur lui même pour permettre du frustum culling.
- Retravailler le noise du terrain pour avoir un relief plus réaliste.
- Generer des heightmaps globale pour définir des continents plus réalistes ou pour customiser le terrain manuellement.
- Au choix : système qui utilise un compute shader pour avoir l'altitude du terrain sur CPU, ou faire un système qui génère une fonction d'altitude en glsl et C++ symétriquement.
- Ajouter une pass de reflection qui capture une version simplifiée de la scene pour completer le SSR (necessaire pour corriger la reflection des atmospheres dans d'autres planètes)
- Shadow maps : Idéallement des Cascaded Shadow Maps. (combiner avec des sphere mask à l'echelle des planètes)

## Problèmes & bugs connus

- Decoupe des UV entre les differentes modulos (vertex->fragment) : Pas très visible, mais réfléchir à un correctif. (en fragment c'est cher)
- Rotation du mesh à déduire des Tangentes / Bitangentes locales : Très problèmatique mais très complexe à résoudre. On verra plus tard.
- Rotation / scale des components cassé dans le scene graph depuis le passage de la caméra à l'origine.
- La déformation des UV sur a sphère fait que les textures sont déformées à certains endroits, gros problème avec les normales qui diviennent incohérentes et qu'il faut compenser.