#%%
from PIL import Image
import numpy as np
import cv2
import matplotlib.pyplot as plt
import struct

NUM_LEDS = 48   # largeur cible -> nb de leds
NUM_STATES = 50 # hauteur cible -> nombre d'états sur un tour (résolution angulaire)


#Nom_fichier = "logo_phelma_rouge.png"
Nom_fichier = "phenix.bmp"

#ouverture fichier
img = Image.open(Nom_fichier)

print(f"Taille originale : {img.size}")       # (largeur, hauteur)
print(f"Mode colorimétrique : {img.mode}")    # RGB, RGBA, L, ...

#%%Affichage de l'image source

plt.figure()
plt.imshow(img)
plt.title("Image source")
plt.show()



#%%conversion RBBA -> RGB
if img.mode == "RGBA":
    r, g, b, a = img.split()
    fond = Image.new("RGB", img.size, (0, 0, 0)) #nouvelle image de la meme taille

    #Quand A=0 → pixel_final = (0, 0, 0) → LED éteinte
    #Quand A=255 → pixel_final = (R, G, B) → couleur intacte 
    fond.paste(img, mask=a)

    img = fond

else:
    img = img.convert("RGB")




#%%Redimmensionnement de l'image : on travail sur une image carrée pour éviter de trop tronquer en POV
#LANCZOS : filtre qui calcule une moyenne pondérée des pixels voisins lors de la réduction

img = img.resize((1000, 1000), Image.LANCZOS)



#%%conversion en image polaire
#img.shape = (hauteur, largeur, canaux)
center = (img.size[0]//2, img.size[1]//2) #centre de l'image
max_radius = min(img.size[0], img.size[1]) // 2  # rayon max

#RGB->BGR (cv2 fonctionne en BGR)
img_polaire = cv2.cvtColor(np.array(img), cv2.COLOR_RGB2BGR)

#conversion polaire
img_polaire = cv2.linearPolar(img_polaire, center, max_radius, cv2.WARP_FILL_OUTLIERS + cv2.INTER_LANCZOS4)
#cv2.WARP_FILL_OUTLIERS -> pixel qui ne correspondent à rien (0,0,0)
#cv2.INTER_LANCZOS4 -> algortihme d'interpolation

#BGR->RGB
img_polaire = cv2.cvtColor(img_polaire, cv2.COLOR_BGR2RGB)

print(img_polaire.shape)

#%%Redimmensionnement de l'image
img_polaire = Image.fromarray(img_polaire) #passage de array a pillow
img_polaire = img_polaire.resize((NUM_LEDS, NUM_STATES), Image.LANCZOS) #redimensionnement
img_polaire = np.array(img_polaire) #on repasse en array


#%%réorganisation POV

#lignes = [ array_ligne0, array_ligne1, array_ligne2, ... array_ligneN ]
#array_lignei = une liste de [R,G,B] * NUM_LEDS
lignes = [img_polaire[i, :, :] for i in range(NUM_STATES)]



#%% affichage POV
TAILLE = 500
cx, cy = TAILLE // 2, TAILLE // 2
R_max = TAILLE // 2

disque = np.zeros((TAILLE, TAILLE, 3), dtype=np.uint8)

for i in range(NUM_STATES):
    theta = i * (2 * np.pi / NUM_STATES)
    for j in range(NUM_LEDS):
        r = j * (R_max / NUM_LEDS)
        x = int(cx + r * np.cos(theta))
        y = int(cy + r * np.sin(theta))
        disque[y, x] = lignes[i][j]

plt.figure(figsize=(6, 6))
plt.imshow(disque, interpolation='nearest')
plt.title("Simulation rendu POV")
plt.axis('off')
plt.show()



#%%Serialisation

image_pov = Nom_fichier[:-4]+".bin"
f = open(image_pov, "wb") #wb = write binaire

# En-tête
f.write(struct.pack(">HB", NUM_STATES, NUM_LEDS)) #conversion en octets
#> → big endian : l'octet de poids fort en premier
#H → unsigned short sur 2 octets → pour NUM_STATES
#B → unsigned byte sur 1 octet → pour NUM_LEDS

# Données
for i in range(NUM_STATES):
    for j in range(NUM_LEDS):
        r, g, b = lignes[i][j]
        f.write(struct.pack("BBB", r, g, b))

f.close()


