using System;
using System.Collections;
using System.Collections.Generic;
using System.Drawing;
using Unity.VisualScripting;
using UnityEngine;

public class PlayerControls : MonoBehaviour
{
    public float moveSpeed = 5f;
    public float mouseSpeed = 1f;
    private Rigidbody rb;
    private Transform cameraView;
    private Vector3 moveInput;
    private float xRotation = 0f;
    private Transform scanner;


    void Start()
    {
        rb = GetComponent<Rigidbody>(); //Dohvati rigidbody igraca
        cameraView = transform.Find("PlayerCamera"); //Dohvati objekt kamere igraca
        scanner = transform.Find("Scanner"); //Dohvati objekt skenera
    }

    void Update()
    {
        float moveX = 0f; //Pomak po X koordinati
        float moveZ = 0f; //Pomak po Z koordinati

        if (Input.GetKey(KeyCode.A)){
            moveX = -1f;
        }
        if (Input.GetKey(KeyCode.S)){
            moveZ = -1f;
        }
        if (Input.GetKey(KeyCode.D)){
            moveX = 1f;
        }
        if (Input.GetKey(KeyCode.W)){
            moveZ = 1f;
        }
        moveInput = new Vector3(moveX, 0f, moveZ).normalized; //Pomak u kojem smjeru

        float mouseX = Input.GetAxis("Mouse X") * mouseSpeed * Time.deltaTime; //Okret kamere u X osi
        float mouseY = Input.GetAxis("Mouse Y") * mouseSpeed * Time.deltaTime; //Okret kamere u Y osi

        xRotation -= mouseY;
        xRotation = Mathf.Clamp(xRotation, -90f, 90f);
        cameraView.localRotation = Quaternion.Euler(xRotation, 0f, 0f); //Okreni kameru za zadani okret misa
        scanner.localRotation = Quaternion.Euler(xRotation, 0f, 0f); //Okreni skener za zadani okret misa
        transform.Rotate(Vector3.up * mouseX); //Okreni igraca za zadani okret misa
    }

    void FixedUpdate()
    {
        Vector3 moveDirection = transform.TransformDirection(moveInput); //U kojem smjeru se krecem
        rb.velocity = moveDirection * moveSpeed + new Vector3(0f, rb.velocity.y, 0f); //Dodaj velocity u tom smjeru
    }
}