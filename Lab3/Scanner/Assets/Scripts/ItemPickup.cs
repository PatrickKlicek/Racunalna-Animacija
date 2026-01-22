using UnityEngine;

public class ItemPickup : MonoBehaviour
{
    public float interactDistance = 3f;
    public GameObject interactText;

    // boolovi za iteme
    public bool hasKljuc;
    public bool hasKocka;
    public bool hasSfera;
    public bool hasStozac;

    private GameObject currentItem;

    void Update()
    {
        Ray ray = new Ray(Camera.main.transform.position, Camera.main.transform.forward);
        RaycastHit hit;

        if (Physics.Raycast(ray, out hit, interactDistance))
        {
            string name = hit.collider.gameObject.name;

            if (name == "Kljuc" || name == "Kocka" || name == "Sfera" || name == "Stozac")
            {
                interactText.SetActive(true);
                currentItem = hit.collider.gameObject;

                if (Input.GetKeyDown(KeyCode.E))
                {
                    PickUpItem(currentItem);
                }
                return;
            }
            if (name == "youwin")
            {
                interactText.SetActive(true);
                currentItem = hit.collider.gameObject;

                if (Input.GetKeyDown(KeyCode.E))
                {
                    Application.Quit();
                }
                return;
            }
        }

        interactText.SetActive(false);
        currentItem = null;
    }

    void PickUpItem(GameObject item)
    {
        switch (item.name)
        {
            case "Kljuc":
                hasKljuc = true;
                break;
            case "Kocka":
                hasKocka = true;
                break;
            case "Sfera":
                hasSfera = true;
                break;
            case "Stozac":
                hasStozac = true;
                break;
        }

        Destroy(item);
        interactText.SetActive(false);
    }
}
