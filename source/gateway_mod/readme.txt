/** \page gateway Gateway

La passerelle est le point d'entrée de toutes les requêtes et remplis plusieurs rôles :
 - Répartir les requêtes entre les diverses instances de puissance et proximité hétérogène (machines en cloud et dédiées)
 - Gérer les erreurs des instances de NAViTiA
 - Récolter les statistiques
 - Effectuer le rendu au format souhaité. En effet le dialogue entre la passerelle et les instances NAViTiA se font au format protocol buffer. Les rendus peuvent-être
    - Protocol Buffer
    - CSV
    - XML
    - JSON
    - HTML

*/
