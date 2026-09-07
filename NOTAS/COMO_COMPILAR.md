# COMO COMPILAR (ENTORNO CORRECTO - no olvidar DEVKITPRO)

## Comando completo (copiar y pegar):
export DEVKITPRO=/opt/devkitpro
export DEVKITPPC=/opt/devkitpro/devkitPPC
make 2>&1 | grep -iE "error:|output" | tail
cp seta-gx.dol /e/apps/ZGX/boot.dol

## CLAVE: hay que setear DEVKITPRO **Y** DEVKITPPC (GCC 15.2.0).
## Si solo se setea DEVKITPPC, el binario compila pero NO funciona
## (los juegos quedan negros) porque no encuentra libogc2/portlibs bien.

## Toolchain correcto: /opt/devkitpro/devkitPPC = GCC 15.2.0 + libogc2.
## NO usar los viejos (devkitPPC_r26 GCC 4.6.3 ni r29 GCC 6.3.0): no linkean
## la libogc2 actual (dan relocation truncated / ABI incompatible).

## El .dol correcto pesa ~1.7MB. Se carga desde /e/apps/ZGX/boot.dol

## COPIAR EL DOL A LOS 2 LUGARES (obligatorio):
cp seta-gx.dol /e/apps/ZGX/boot.dol
cp seta-gx.dol "/e/wiiflow/plugins/Sega Saturn/ZGX.dol"
## (o usar: ./copiar_dol.sh)
## apps/ZGX = carga directa (Homebrew Channel)
## wiiflow/plugins/Sega Saturn = carga desde WiiFlow
