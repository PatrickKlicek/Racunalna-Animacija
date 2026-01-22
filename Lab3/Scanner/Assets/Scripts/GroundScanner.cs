using UnityEngine;

public class GroundScanner : MonoBehaviour
{
    public Scanner playerScanner;  // Scanner koji je u rukama
    public float pickupDistance = 2f;
    public KeyCode pickupKey = KeyCode.E;

    private bool pickedUp = false;
    private Scanner tempScanner; // Scanner na podu

    void Start()
    {
        tempScanner = GetComponent<Scanner>(); // ovo mora postojati na objektu
        if (tempScanner != null)
            tempScanner.enabled = true; // stalno scan dok je na podu

        if (playerScanner != null)
            playerScanner.enabled = false; // u rukama iskljuèen
    }

    void Update()
    {
        if (pickedUp) return;

        GameObject player = GameObject.FindWithTag("Player");
        if (player == null) return;

        float dist = Vector3.Distance(transform.position, player.transform.position);

        if (dist <= pickupDistance && Input.GetKeyDown(pickupKey))
        {
            pickedUp = true;

            if (tempScanner != null)
                tempScanner.enabled = false;

            if (playerScanner != null)
                playerScanner.enabled = true;

            gameObject.SetActive(false);
        }
    }
}
