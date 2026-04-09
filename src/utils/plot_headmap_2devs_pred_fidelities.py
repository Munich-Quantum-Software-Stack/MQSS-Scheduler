import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
import pandas as pd

def plot_heatmap_with_real_dev(mae_df, r2_df, use_strong_green_diverging=True):
    """
    Plot heatmap of R² with MAE+R² annotations.
        - use_strong_green_diverging=True uses 'RdYlGn' (red->yellow->green) centered at 0
    so negative R² appear red and positive green.
        - If you want a purely green sequential colormap instead, pass False and it will use 'Greens'.
    """
    plt.figure(figsize=(8, 4))
    # cmap = "RdYlGn" if use_strong_green_diverging else "Greens"
    # cmap = sns.light_palette("green", as_cmap=True)
    # cmap = sns.cubehelix_palette(as_cmap=True)
    ax = sns.heatmap(
        r2_df,
        annot=False,
        # cmap=cmap,
        center=0 if use_strong_green_diverging else None,
        linewidths=0.5,
        linecolor="gray",
        cbar_kws={"label": "R² Score", "shrink": 0.8},
    )

    # nicer font sizes for ticks (optional)
    ax.tick_params(axis="x", labelsize=10, rotation=0)
    ax.tick_params(axis="y", labelsize=10, rotation=0)

    # annotate cells with both MAE and R²
    for y in range(r2_df.shape[0]):
        for x in range(r2_df.shape[1]):
            backend = r2_df.index[y]
            model = r2_df.columns[x]
            r2_val = r2_df.iloc[y, x]
            mae_val = mae_df.loc[backend, model]
            text = f"MAE: {mae_val:.3f}\nR²: {r2_val:.3f}"

            ax.text(
                x + 0.5,
                y + 0.5,
                text,
                ha="center",
                va="center",
                color="black",
                fontsize=8,
                fontweight="bold",
            )

    # Larger, bold axis titles
    ax.set_xlabel("Real Backends installed at Our Local Compute Center", fontsize=10, fontweight="bold")
    ax.set_ylabel("Models", fontsize=10, fontweight="bold")

    # Bigger title
    # plt.title("Model Performance per real Device (MAE and R²)", fontsize=16, fontweight="bold")

    # Make colorbar label bold & readable
    cbar = ax.collections[0].colorbar
    if cbar is not None:
        cbar.set_label("R² Score", fontsize=12, fontweight="bold")
        cbar.ax.tick_params(labelsize=10)

    plt.tight_layout()
    plt.savefig("./heatmap_2dev_pred_fid.pdf")
    # plt.show()

if __name__ == "__main__":
    # Example usage with dummy data
    mae_df = pd.DataFrame({
        'device1': [0.050610, 0.06310, 0.053413, 0.051267, 0.060887, 0.048280],
        'device2': [0.032051, 0.05223, 0.033668, 0.032216, 0.048318, 0.029235]
    }, index=['Random_Forest', 'SVM', 'KNN', 'basic_MLP', 'masked_loss_MLP', 'dual_head_MLP'])

    r2_df = pd.DataFrame({
        'device1': [0.941737, 0.939583, 0.939474, 0.943717, 0.937317, 0.945406],
        'device2': [0.984502, 0.973486, 0.982720, 0.985837, 0.975153, 0.985852]
    }, index=['Random_Forest', 'SVM', 'KNN', 'basic_MLP', 'masked_loss_MLP', 'dual_head_MLP'])

    plot_heatmap_with_real_dev(mae_df, r2_df)