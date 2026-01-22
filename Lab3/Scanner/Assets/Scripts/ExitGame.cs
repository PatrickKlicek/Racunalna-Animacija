using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ExitGame : MonoBehaviour
{
    public GameObject interactText;
    private bool playerInRange = false;
    void OnTriggerEnter(Collider other)
    {
        if (other.CompareTag("Player"))
        {
            Debug.Log("Ušao je: " + other.name);
            playerInRange = true;
            if (interactText != null)
                interactText.SetActive(true);
        }
    }

    void OnTriggerExit(Collider other)
    {
        if (other.CompareTag("Player"))
        {
            Debug.Log("Izašo je: " + other.name);
            playerInRange = false;
            if (interactText != null)
                interactText.SetActive(false);
        }
    }

    void Update()
    {
        if (playerInRange && Input.GetKeyDown(KeyCode.E))
        {
            Application.Quit();
        }
    }
}
