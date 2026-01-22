using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class AllOpen : MonoBehaviour
{
    public ItemPickup playerPickup;

    void Update()
    {
        if (playerPickup.hasKljuc &&
            playerPickup.hasKocka &&
            playerPickup.hasSfera &&
            playerPickup.hasStozac)
        {
            Destroy(gameObject);
        }
    }
}
