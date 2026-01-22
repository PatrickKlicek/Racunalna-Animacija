using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class KeyOpen : MonoBehaviour
{
    public ItemPickup playerPickup; // referenca na ItemPickup skriptu

    void Update()
    {
        if (playerPickup.hasKljuc)
        {
            Destroy(gameObject);
        }
    }
}
