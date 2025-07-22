from concurrent.futures import thread
# from operator import index
import ROOT
# from numpy import true_divide
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

# Prepare an input tree to run on
treeName = "d3dEvtTree"
fileName = "/data3/TN-Dose3D/bartek/dose3d-geant4-linac/build/run-0-analysis.root"

if __name__=="__main__":
    sns.set() # Setting seaborn as default style even if use only matplotlib
    ROOT.EnableImplicitMT(4); #Enable ROOT's implicit multi-threading
    print("Processing NTuple data to Pandas Data Frame...")
    # https://www.easytweaks.com/pandas-group-one-multiple-columns/
    rdf = ROOT.RDataFrame(treeName, fileName)
    threads = [*range(8)]
    print(threads)

    threadRdf = rdf.Filter(f"ThreadId=={0}")
    treadEntries = threadRdf.Count().GetValue()
    print(f"#Entries in thread({treadEntries})")
    # access the data from Python as Numpy arrays.
    # The returned object is a dictionary with the column names as keys and 1D numpy arrays with
    # the content as values.
    # df = pd.DataFrame(threadRdf.AsNumpy())
    df = pd.DataFrame(rdf.AsNumpy())
    # print(df)

    #cellid_to_position_mapping = pd.read_csv("/data3/TN-Dose3D/bartek/dose3d-geant4-linac/build/dose3d_4x4x4_pmma_cellIdPosMapping.csv") 
    #print(cellid_to_position_mapping)
    dvmin=0
    dvmax=9 * 1e-5

    fig, axes = plt.subplots(1, 4, figsize=(18, 3))

    # Grouping a pandas.DataFrame by columns a and b creates groups consisting of rows which have the same value in a and b.s
    df_z0 = df[df["CellIdZ"]==0]
    df_gr_z0 = df_z0.groupby(['CellIdX','CellIdY','CellIdZ']).agg(CellDose = ("Dose","sum")).reset_index()
    print(df_gr_z0)
    # doses_z0 = pd.pivot_table(df_gr_z0,index="CellIdX",columns="CellIdY",values="CellDose")
    # print(df_gr_z0)
    # #sns.heatmap(ax=axes[0], data=doses_z0, annot=True, annot_kws={"fontsize":8}, cmap="coolwarm", vmin=dvmin,vmax=dvmax)
    # #axes[0].set_title("CellIdZ=0")

    # df_z1 = df[df["CellIdZ"]==1]
    # df_gr_z1 = df_z1.groupby(['CellIdX','CellIdY','CellIdZ']).agg(CellDose = ("Dose","sum"))
    # doses_z1 = pd.pivot_table(df_gr_z1,index="CellIdX",columns="CellIdY",values="CellDose")
    # #sns.heatmap(ax=axes[1], data=doses_z1, annot=True, annot_kws={"fontsize":8}, cmap="coolwarm", vmin=dvmin,vmax=dvmax)
    # #axes[1].set_title("CellIdZ=1")

    # df_z2 = df[df["CellIdZ"]==2]
    # df_gr_z2 = df_z2.groupby(['CellIdX','CellIdY','CellIdZ']).agg(CellDose = ("Dose","sum"))
    # doses_z2 = pd.pivot_table(df_gr_z2,index="CellIdX",columns="CellIdY",values="CellDose")
    # #sns.heatmap(ax=axes[2], data=doses_z2, annot=True, annot_kws={"fontsize":8}, cmap="coolwarm",vmin=dvmin,vmax=dvmax)
    # #axes[2].set_title("CellIdZ=2")

    # df_z3 = df[df["CellIdZ"]==3]
    # df_gr_z3 = df_z3.groupby(['CellIdX','CellIdY','CellIdZ']).agg(CellDose = ("Dose","sum"))
    # doses_z3 = pd.pivot_table(df_gr_z3,index="CellIdX",columns="CellIdY",values="CellDose")
    # #sns.heatmap(ax=axes[3], data=doses_z3, annot=True, annot_kws={"fontsize":8}, cmap="coolwarm",vmin=dvmin,vmax=dvmax)
    # #axes[3].set_title("CellIdZ=3")

    # #plt.show()

    # df_all = pd.concat([df_gr_z0,df_gr_z1,df_gr_z2,df_gr_z3],axis=0,ignore_index=False,verify_integrity=False)
    # # print(df_all)
    # df_all.to_csv('doses.csv', encoding='utf-8',index=False)