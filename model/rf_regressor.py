#Data Load

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from sklearn.model_selection import train_test_split
from sklearn.ensemble import RandomForestRegressor
from sklearn.ensemble import BaggingRegressor
from sklearn.metrics import r2_score
from sklearn.metrics import mean_squared_error
from sklearn.metrics import mean_absolute_percentage_error
from sklearn.model_selection import cross_val_score

pd.set_option('display.max.colwidth', 50)
pd.set_option('display.width', 1000)

raw_dataset = pd.read_csv('./data/5_PA_2.csv', header=0, index_col=False)
print("* Raw dataset size : ", len(raw_dataset))

dataset = raw_dataset[["weight", "failure", "set_nozzle_temperature", "set_front_temperature", "set_intermediate_temperature", "set_rear_temperature", "set_mold_velocity_1", "set_mold_velocity_2", "set_mold_velocity_3", "set_mold_velocity_4", "set_mold_velocity_5", "set_mold_pressure_1", "set_mold_pressure_2", "set_mold_pressure_3", "set_mold_pressure_4", "set_mold_pressure_5", "set_mold_position_1", "set_mold_position_2", "set_mold_position_3", "set_mold_position_4", "set_mold_position_5", "set_hold_velocity_1", "set_hold_velocity_2", "set_hold_velocity_3", "set_hold_pressure_1", "set_hold_pressure_2", "set_hold_pressure_3"]]

positive_set = dataset.where(dataset["failure"]==0).dropna()
print("> Positive Data : ", len(positive_set))
positive_X = positive_set[["set_nozzle_temperature", "set_front_temperature", "set_intermediate_temperature", "set_rear_temperature", "set_mold_velocity_1", "set_mold_velocity_2", "set_mold_velocity_3", "set_mold_velocity_4", "set_mold_velocity_5", "set_mold_pressure_1", "set_mold_pressure_2", "set_mold_pressure_3", "set_mold_pressure_4", "set_mold_pressure_5", "set_mold_position_1", "set_mold_position_2", "set_mold_position_3", "set_mold_position_4", "set_mold_position_5", "set_hold_velocity_1", "set_hold_velocity_2", "set_hold_velocity_3", "set_hold_pressure_1", "set_hold_pressure_2", "set_hold_pressure_3"]]
print("> Positive X : ", len(positive_X))

negative_set = dataset.where(dataset["failure"]==1).dropna()
print("> Negative Data : ", len(negative_set))
negative_X = negative_set[["set_nozzle_temperature", "set_front_temperature", "set_intermediate_temperature", "set_rear_temperature", "set_mold_velocity_1", "set_mold_velocity_2", "set_mold_velocity_3", "set_mold_velocity_4", "set_mold_velocity_5", "set_mold_pressure_1", "set_mold_pressure_2", "set_mold_pressure_3", "set_mold_pressure_4", "set_mold_pressure_5", "set_mold_position_1", "set_mold_position_2", "set_mold_position_3", "set_mold_position_4", "set_mold_position_5", "set_hold_velocity_1", "set_hold_velocity_2", "set_hold_velocity_3", "set_hold_pressure_1", "set_hold_pressure_2", "set_hold_pressure_3"]]
print("> Negative X : ", len(negative_X))

positive_y = positive_set[["weight"]]
print("> Positive y : ", len(positive_y))
negative_y = negative_set[["weight"]]
print("> Negative y : ", len(negative_y))

data_columns = positive_X.columns.to_numpy()


X_train, X_test, y_train, y_test = train_test_split(positive_X, np.ravel(positive_y), test_size=0.33, shuffle=True)
model = RandomForestRegressor(max_depth=10, max_features=5, oob_score=False, bootstrap=False).fit(X_train, y_train)

y_pred_trainset = model.predict(X_train)
y_pred_testset = model.predict(X_test)


print("* K-fold Cross validation scores :")
scores = cross_val_score(model, positive_X, np.ravel(positive_y), cv=5, scoring="neg_mean_absolute_percentage_error")*-100
print(scores)
print("* Average MAPE : %0.2f %%"%scores.mean())

print("* Trainset R2 Score :", (r2_score(y_train, y_pred_trainset))) 
print("* Testset R2 Score :", (r2_score(y_test, y_pred_testset)))