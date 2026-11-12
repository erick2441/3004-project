# DragonEndPOS Deliverable 2 TODO

## 8. Cancel an Online Order

See the Project Overview, Section 2 (Customer Online Order and Kitchen Order Queue Process) and Section 3 (Business Rules).

- [ ] Let the customer cancel their current order from the online dashboard while it is In Progress or Completed. Ask for confirmation before cancelling.
- [ ] Set the status to Cancelled and record the cancellation date and time.
- [ ] Remove an In Progress order from the kitchen queue or a Completed order from the cashier's pickup list.
- [ ] Return the amount charged as account credit, restore any account credit used, return any loyalty points redeemed, and remove any loyalty points earned on the order.
- [ ] Show the cancelled order and the account credit received in the customer's order history.
- [ ] Prevent cancellation of Picked Up and Cancelled orders.

## Cashier features

- [ ] Make features 9 and 10 available to cashiers after identification on the startup form.

### 9. View Orders Ready for Pickup

See the Project Overview, Section 2 (Staff Cashier).

- [ ] List only Completed orders, showing each order's number, customer name, items purchased, and order date and time.

### 10. Mark an Order as Picked Up

See the Project Overview, Section 2 (Staff Cashier) and Section 3 (Business Rules).

- [ ] Let the cashier select a Completed order, mark it Picked Up, and record the pickup date and time.
- [ ] Remove the order from the pickup list and add it to the customer's order history.

## Manager features

- [ ] Make features 11 and 12 available to managers after identification on the startup form.

### 11. View Menu Items

See the Project Overview, Section 2 (Manager).

- [ ] Group all menu items by category and display each item's name, category, price, and required ingredients with their quantities.

### 12. Change a Menu Item Price

See the Project Overview, Section 2 (Manager).

- [ ] Allow changes to the price of an existing menu item only. Keep item creation, removal, and edits to name, category, and ingredients unavailable.
- [ ] Require a price greater than $0.00 with at most two decimal places. Show an error for invalid input and leave the saved price unchanged.
- [ ] Apply the new price only to future orders. Preserve the price charged on existing orders and in order history.

## Cross-role updates and persistence

- [ ] Show changes made by customers, kitchen staff, cashiers, and managers in the relevant interfaces when other users enter the system.
- [ ] Verify that those changes persist after the program restarts.
